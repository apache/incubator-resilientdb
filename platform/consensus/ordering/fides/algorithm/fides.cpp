#include "platform/consensus/ordering/fides/algorithm/fides.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace fides {

Fides::Fides(int id, int f, int total_num, SignatureVerifier* verifier, const ResDBConfig& config, oe_enclave_t* enclave)
    : ProtocolBase(id, f, total_num), verifier_(verifier), config_(config), enclave_(enclave) {
  // TODO: 5.Quorum to f+1 - Done
  limit_count_ = f + 1;
  batch_size_ = 5;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_, enclave);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  send_thread_ = std::thread(&Fides::AsyncSend, this);
  commit_thread_ = std::thread(&Fides::AsyncCommit, this);
  execute_thread_ = std::thread(&Fides::AsyncExecute, this);
  cert_thread_ = std::thread(&Fides::AsyncProcessCert, this);

  global_stats_ = Stats::GetGlobalStats();

  LOG(ERROR) << "id:" << id << " f:" << f << " failureNum:" << config_.GetFailureNum() << " total:" << total_num;
}

Fides::~Fides() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if(cert_thread_.joinable()){
    cert_thread_.join();
  }
}

// TODO: 1.Change this to Using trusted global random
// TODO: Not every round has a different leader
int Fides::GetLeader(int r) {
  // return r / 2 % total_num_ + 1;
  {
    std::unique_lock<std::mutex> lk(leader_list_mutex_);
    if (leader_list_.count(r) != 0) {
      return leader_list_[r];
    }
    if (r % 3 != 0) {
      leader_list_[r] = leader_list_[r-1];
      return leader_list_[r];
    }
  }

  int ret;
  uint32_t randNum;
  
  // Send f+1 counter value and attestation to proof can obtain new random number
  std::vector<CounterInfo> counters;
  if(r > 0) {
    // LOG(ERROR)<<"In GetLeader. round:"<<r;
    counters = proposal_manager_->GetCounterFromRound(r - 1);
    assert(counters.size()>=limit_count_);
  }

  size_t previous_cert_size = counters.size();
  unsigned char** attestation_array = new unsigned char*[previous_cert_size];
  size_t* attestation_size_array = new size_t[previous_cert_size];
  uint32_t* counter_value_array = new uint32_t[previous_cert_size];
  int i = 0;

  std::string serialized_data;
  for (size_t i = 0; i < counters.size(); ++i) {
    // LOG(ERROR)<<"attestation is: "<<link.counter().attestation();
    // set counter value
    counter_value_array[i] = counters[i].value();

    // set counter attestation
    serialized_data = counters[i].attestation();
    size_t size = serialized_data.size();

    attestation_array[i] = new unsigned char[size];
    std::memcpy(attestation_array[i], serialized_data.data(), size);
    attestation_size_array[i] = size;
  }

  result = generate_rand(enclave_, &ret, previous_cert_size, limit_count_, 
                        counter_value_array, attestation_size_array, 
                        attestation_array, &randNum);
  
  if (result != OE_OK || ret != 0) {
    LOG(ERROR) << "Function generate_rand failed with " << ret;
  }

  // LOG(ERROR) << "newLeader:" << leader_list_[r];
  std::unique_lock<std::mutex> lk(leader_list_mutex_);
  leader_list_[r] = randNum + 1;
  return leader_list_[r];
}


void Fides::AsyncSend() {

  uint64_t last_time = 0;
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      if(start_){
        //LOG(ERROR)<<"not enough txn";
        //break;
      }
      continue;
    }

    queue_size_--;
    //LOG(ERROR)<<"txn before waiting time:"<<GetCurrentTime() - txn->create_time()<<" queue size:"<<queue_size_;
    while (!IsStop()) {
      int current_round = proposal_manager_->CurrentRound();
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(10000),
                        [&] { return proposal_manager_->Ready(); });

      if (proposal_manager_->Ready()) {
        start_ = 1;
        break;
      }
    }

    //LOG(ERROR)<<"txn waiting time:"<<GetCurrentTime() - txn->create_time();
    txn->set_queuing_time(GetCurrentTime() - txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
    global_stats_->AddRoundLatency(GetCurrentTime() - last_time);
    last_time = GetCurrentTime();

    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto txn = txns_.Pop(10);
      if (txn == nullptr) {
        continue;
        break;
      }
      txn->set_queuing_time(GetCurrentTime() - txn->create_time());
      queue_size_--;
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }

    global_stats_->ConsumeTransactions(queue_size_);

    auto proposal = proposal_manager_->GenerateProposal(txns);
    Broadcast(MessageType::NewBlock, *proposal);

    std::string block_data;
    proposal->SerializeToString(&block_data);
    global_stats_->AddBlockSize(block_data.size());

    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound() - 1;
    //LOG(ERROR) << "bc txn:" << txns.size() << " round:" << round;
    // TODO: change commit rule here
    if (round > 1) {
      if (round % 3 == 0) {
        CommitRound(round - 3);
      }
    }
  }
}

// TODO: Change commit rule here.
void Fides::AsyncCommit() {
  int previous_round = -3;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      continue;
    }

    int round = *round_or;
    //int64_t start_time = GetCurrentTime();
    //LOG(ERROR) << "commit round:" << round;

    int new_round = previous_round;
    for (int r = previous_round + 3; r <= round; r += 3) {
      int leader = GetLeader(r);
      const Proposal * req = nullptr;
      while(!IsStop()){
        req = proposal_manager_->GetRequest(r, leader);
        // req:"<<(req==nullptr);
        if (req == nullptr) {
          //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" not exit";
          //usleep(1000);
          std::unique_lock<std::mutex> lk(mutex_);
          vote_cv_.wait_for(lk, std::chrono::microseconds(100),
              [&] { return true; });

          continue;
        }
        break;
      }
      //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" delay:"<<(GetCurrentTime() - req->header().create_time());
      int reference_num = proposal_manager_->GetReferenceNum(*req);
      //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" ref:"<<reference_num;
      if (leader <= config_.GetFailureNum()) {
        // LOG(ERROR)<<"Faulty Leader.";
        reference_num = 0;
      }
      if (reference_num < limit_count_) {
        continue;
      } else {
        // LOG(ERROR) << "go to commit r:" << r << " previous:" <<
        // previous_round;
        for (int j = new_round + 3; j <= r; j += 3) {
          int pre_leader = GetLeader(j);

          auto req = proposal_manager_->GetRequest(j, pre_leader);
          if (req == nullptr) {
            continue;
          }
          int reference_num = proposal_manager_->GetReferenceNum(*req);
          if (reference_num < limit_count_) {
            continue;
          }

          CommitProposal(j, pre_leader);
        }
      }
      new_round = r;
    }
    //int64_t end_time = GetCurrentTime();
    //global_stats_->AddCommitRuntime(end_time-commit_time);
    previous_round = new_round;
  }
}

// TODO: 3.Decrypt the request before executing.
void Fides::AsyncExecute() {
int64_t last_commit_time = 0;
int last_round = 0;
  while (!IsStop()) {
    std::unique_ptr<Proposal> proposal = execute_queue_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    int commit_round = proposal->header().round();
    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - proposal->queuing_time();
    global_stats_->AddCommitQueuingLatency(waiting_time);
    //global_stats_->AddCommitInterval(commit_time - last_commit_time);
    //last_commit_time = commit_time;
    std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
    std::queue<std::unique_ptr<Proposal>> q;
    q.push(std::move(proposal));

    //LOG(ERROR)<<" commit round:"<<commit_round;
    //proposal_manager_->SetCommittedRound(commit_round);

    //global_stats_->AddCommitRoundLatency(commit_round - last_round);
    //last_round = commit_round;

    //int64_t start_time = GetCurrentTime();
    while (!q.empty()) {
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }


      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          // LOG(ERROR)<<"no data round:"<<link_round<<" link
          // proposer:"<<link_proposer;
          continue;
        }
        q.push(std::move(next_p));
      }
      ps[p->header().round()].push_back(std::move(p));
    }

    int num = 0;
    int pro = 0;
    for (auto& it : ps) {
      for (auto& p : it.second) {
        //LOG(ERROR) << "=============== commit proposal round :"
        //           << p->header().round()
        //           << " header round:"<<p->header().round()
        //           << " commit round:"<<commit_round
        //           << " proposer:" << p->header().proposer_id()
        //           << " transaction size:" << p->transactions_size()
        //           << " commit time:"
        //           << (GetCurrentTime() - p->header().create_time())
        //           << " create time:" << p->header().create_time()
        //           <<" execute id:"<<execute_id_ 
        //          <<" round delay:"<<(commit_round - p->header().round());

        global_stats_->AddCommitLatency(commit_time - p->header().create_time() - waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round - p->header().round());
        
        // BatchUserRequest batch_request; // For transaction decryption
        for (auto& tx : *p->mutable_transactions()) {
          tx.set_id(execute_id_++);
          num++;
          // LOG(ERROR)<<" commit txn create time 1:"<<tx.create_time();
          /* // For transaction decryption
          // Decrypt request data here.
          if (!batch_request.ParseFromString(tx.data())) {
            LOG(ERROR) << "parse data fail in ParseData!";
            // return nullptr;
          }
          for (int i = 0; i < batch_request.user_requests_size(); ++i) {
            // 获取当前的 UserRequest
            Request user_request = batch_request.user_requests(i).request();
            // LOG(ERROR) << "user_request.data(): "<<user_request.data();
            // LOG(ERROR) << "user_request.encrypted_data(): "<<user_request.encrypted_data();
            int ret = 0;
            unsigned char * decrypted_data = new unsigned char[key_size];
            size_t decrypted_len;
            const std::string& encrypted_data = user_request.encrypted_data();
            unsigned char* request_data = reinterpret_cast<unsigned char*>(const_cast<char*>(encrypted_data.c_str()));
            
            // result = decrypt(enclave_, &ret, request_data, &decrypted_data, encrypted_data.size(), &decrypted_len);
            // if (result != OE_OK || ret != 0) {
            //     LOG(ERROR) << "Host: decrypt failed with " << ret;
            //     LOG(ERROR) << "encrypted_data: "<<encrypted_data<<" size: "<<encrypted_data.size();
            // } else {
            //     LOG(ERROR) << "encrypted_data: "<<encrypted_data<<" size: "<<encrypted_data.size();
            // }
            std::string decrypted_request(decrypted_data, decrypted_data + decrypted_len);
            // LOG(ERROR) << "After decryption: "<< decrypted_request;

            batch_request.mutable_user_requests(i)->mutable_request()->set_encrypted_data("");
            batch_request.mutable_user_requests(i)->mutable_request()->set_data(decrypted_request);
            // user_request->set_data(decrypted_request);
            // usleep(2000);
          }
          std::string new_tx_data;
          batch_request.SerializeToString(&new_tx_data);
          tx.set_data(new_tx_data);
          */
          Commit(tx);
        }
        pro++;
      }
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddCommitRuntime(end_time-commit_time);
    global_stats_->AddCommitTxn(num);
    global_stats_->AddCommitBlock(pro);
  }
}

void Fides::CommitProposal(int round, int proposer) {
  //LOG(ERROR) << "commit round:" << round << " proposer:" << proposer;

  //int64_t last_commit_time = 0;
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;

  std::unique_ptr<Proposal> p =
      proposal_manager_->FetchRequest(round, proposer);
  if(p==nullptr){
    LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer<<" not exist";
    assert(1==0);
    return;
  }
  assert(p !=nullptr);
  p->set_queuing_time(GetCurrentTime());
  execute_queue_.Push(std::move(p));
}

void Fides::CommitRound(int round) {
  //LOG(ERROR)<<" commit round:"<<round;
  commit_queue_.Push(std::make_unique<int>(round));
}

bool Fides::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  LOG(ERROR)<<"recv txn";
  txn->set_create_time(GetCurrentTime());
  txns_.Push(std::move(txn));
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Fides::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
  //           << " round:" << proposal->header().round();
  int proposer_id = proposal->header().proposer_id();
  int round = proposal->header().round();
  std::string hash = proposal->hash();

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    proposal->set_queuing_time(GetCurrentTime());
    //std::unique_lock<std::mutex> lk(check_block_mutex_);
    // Verify the block, including the counter
    if(!CheckBlock(*proposal)){
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      //LOG(ERROR)<<"add future block:"<<proposal->header().round()<<" sender:"<<proposal->header().proposer_id();
      future_block_[proposal->header().round()][proposal->hash()] = std::move(proposal);
      return false;
    }
  }

  // Generate certificate
  auto cert = std::make_unique<Certificate>();
  global_stats_->AddExecutePrepareDelay(GetCurrentTime() - proposal->header().create_time());
  //global_stats_->AddCommitLatency(GetCurrentTime() - proposal->header().create_time());
  
  cert->set_hash(hash);
  cert->set_round(round);
  cert->set_proposer(proposer_id);
  *cert->mutable_strong_cert() = proposal->header().strong_cert();
  *cert->mutable_counter() = proposal->header().counter();

  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    //LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
    //         << " round:" << proposal->header().round();
     global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
     proposal->set_queuing_time(0);
     {
      // std::unique_lock<std::mutex> lk(check_block_mutex_);
      proposal_manager_->AddBlock(std::move(proposal));
      //  CheckFutureCert(round, hash, proposer);
     }
  }

  ReceiveBlockCert(std::move(cert));

  return true; 
  // return SendBlockAck(std::move(proposal));
}

/*
void Fides::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  //LOG(ERROR) << "recv block ack from:" << sender << " num:" << received_num_[hash].size()<<" round:"<<round;
  if (received_num_[hash].size() == limit_count_) {
    Certificate cert;
    for (auto& it : received_num_[hash]) {
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    const Proposal* p = proposal_manager_->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    global_stats_->AddExecutePrepareDelay(GetCurrentTime() - p->header().create_time());
    //global_stats_->AddCommitLatency(GetCurrentTime() - p->header().create_time());
    *cert.mutable_strong_cert() = p->header().strong_cert();
    //LOG(ERROR)<<"send cert, round:"<<p->header().round();
    Broadcast(MessageType::Cert, cert);
  }
}
*/

bool Fides::VerifyCert(const Certificate & cert){
  // std::map<std::string, int> hash_num;
  // bool vote_num = false;
  // for(auto& metadata: cert.metadata()){
  //   bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
  //   if(!valid){
  //     return false;
  //   }
  //   hash_num[metadata.hash()]++;
  //   if(hash_num[metadata.hash()]>=2*f_+1){
  //     vote_num = true;
  //   }
  // }
  // return vote_num;
  // TODO: change quorum here
  return true;
}

bool Fides::VerifyCounter(int round, const CounterInfo& counter) {
  std::string attestation = counter.attestation();
  int counter_value = counter.value();
  if (attestation != "Fake Attestation" || counter_value != round) {
    return false;
  }
  return true;
}

bool Fides::CheckBlock(const Proposal& p){
  for(auto& link : p.header().strong_cert().cert()){
    int link_round = link.round();
    int link_proposer = link.proposer();
    if(!proposal_manager_->CheckCert(link_round, link_proposer)){
      //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
      return false;
    }
  }

  for(auto& link : p.header().weak_cert().cert()){
    int link_round = link.round();
    int link_proposer = link.proposer();
    if(!proposal_manager_->CheckCert(link_round, link_proposer)){
      //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" weak link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
      return false;
    }
  }

  if(!VerifyCounter(p.header().round(), p.header().counter())){
    //LOG(ERROR)<<" check block round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link:"<<link_proposer<<" link round:"<<link_round<<" not exist";
    return false;
  }
  //LOG(ERROR)<<" check cert round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link size:"<<p.header().strong_cert().cert_size()<<" weak link size:"<<p.header().weak_cert().cert_size();
  return true;
}

bool Fides::CheckCert(const Certificate& cert){
  if(!proposal_manager_->CheckBlock(cert.hash())){
    //future_cert_[cert->round()][cert->hash()] = std::move(cert);
    return false;
  }
  return true;
}

/*
bool Fides::SendBlockAck(std::unique_ptr<Proposal> proposal) {
  
  Metadata metadata;
  metadata.set_sender(id_);
  metadata.set_hash(proposal->hash());
  metadata.set_round(proposal->header().round());
  metadata.set_proposer(proposal->header().proposer_id());

  std::string data_str = proposal->hash();
  auto hash_signature_or = verifier_->SignMessage(data_str);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *metadata.mutable_sign()=*hash_signature_or;
  metadata.set_sender(id_);

  {
    //std::unique_lock<std::mutex> lk(txn_mutex_);
    //LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
    //         << " round:" << proposal->header().round();
     int round = proposal->header().round();
     std::string hash = proposal->hash();
     int proposer = proposal->header().proposer_id();
     global_stats_->AddCommitWaitingLatency(GetCurrentTime() - proposal->queuing_time());
     proposal->set_queuing_time(0);
     {
      //std::unique_lock<std::mutex> lk(check_block_mutex_);
       proposal_manager_->AddBlock(std::move(proposal));
       CheckFutureCert(round, hash, proposer);
     }
  }
  // SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}
*/


void Fides::CheckFutureBlock(int round){
  //LOG(ERROR)<<" check block round from cert:"<<round;
  std::map<std::string,std::unique_ptr<Proposal>> hashs;
  {
    std::unique_lock<std::mutex> clk(check_block_mutex_);

    {
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      if(future_block_.find(round) == future_block_.end()){
        return;
      }
      for(auto& it : future_block_[round]){
        if(CheckBlock(*it.second)){
          LOG(ERROR)<<" add new block round:"<<it.second->header().round();
          hashs[it.first] = std::move(it.second);
        }
      }

      LOG(ERROR)<<" check round:"<<round<<" hash size:"<<hashs.size();
      for(auto& it : hashs){
        future_block_[round].erase(future_block_[round].find(it.first));
      }

      if(future_block_[round].size() == 0){
        future_block_.erase(future_block_.find(round));
      }
    }
  }

  for(auto& it : hashs){
    //LOG(ERROR)<<"send future block round:"<<it.second->header().round()<<" proposer:"<<it.second->header().proposer_id();
    int block_round = it.second->header().round();
    //std::unique_lock<std::mutex> lk(check_block_mutex_);
    {
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      if(future_cert_[block_round].find(it.first) != future_cert_[block_round].end()){
        std::unique_ptr<Certificate> cert = std::move(future_cert_[block_round][it.first]);
        //LOG(ERROR)<<" add new cert:"<<block_round<<" from:"<<cert->proposer();
        cert_queue_.Push(std::move(cert));
        future_cert_[block_round].erase(future_cert_[block_round].find(it.first));
        if(future_cert_[block_round].size() == 0){
          future_cert_.erase(future_cert_.find(block_round));
        }
      }
    }
    
    //LOG(ERROR)<<" Redo the proposal again, round: "<<block_round;
    // Redo the proposal again.
    ReceiveBlock(std::move(it.second));
  }
}



/* This situation will not happen
void Fides::CheckFutureCert(int round, const std::string& hash, int proposer) {
  std::unique_lock<std::mutex> lk(future_cert_mutex_);
  auto it = future_cert_[round].find(hash);
  if(it != future_cert_[round].end()){
    //LOG(ERROR)<<" add back future cert, round:"<<round<<" proposer:"<<proposer;
    cert_queue_.Push(std::move(it->second));
    future_cert_[round].erase(it); 
    if(future_cert_[round].empty()){
      future_cert_.erase(future_cert_.find(round));
    }
  }
}
*/

void Fides::AsyncProcessCert(){
  while(!IsStop()){
    std::unique_ptr<Certificate> cert = cert_queue_.Pop();
    if(cert == nullptr){
      continue;
    }

    int round = cert->round();
    int cert_sender = cert->proposer();

    //LOG(ERROR)<<" add cert, round:"<<round<<" from:"<<cert_sender;
    {
      std::unique_lock<std::mutex> clk(check_block_mutex_);
      proposal_manager_->AddCert(std::move(cert));
      {
        std::unique_lock<std::mutex> lk(mutex_);
        vote_cv_.notify_all();
      }
    }
    CheckFutureBlock(round+1);
  }
}

void Fides::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if(!VerifyCert(*cert)){
    assert(1==0);
    return;
  }

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    if(!CheckCert(*cert)){
      //LOG(ERROR)<<" add future cert round:"<<cert->round()<<" proposer:"<<cert->proposer();
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      future_cert_[cert->round()][cert->hash()] = std::move(cert);
      return;
    }
  }
  
  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time-start_time); 

  cert_queue_.Push(std::move(cert));
}

}  // namespace fides
}  // namespace resdb
