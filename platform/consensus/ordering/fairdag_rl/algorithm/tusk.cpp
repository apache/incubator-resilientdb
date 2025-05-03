#include "platform/consensus/ordering/fairdag_rl/algorithm/tusk.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag_rl {

Tusk::Tusk(int id, int f, int total_num, SignatureVerifier* verifier,
          common::ProtocolBase::SingleCallFuncType single_call,  
          common::ProtocolBase :: BroadcastCallFuncType broadcast_call,
          std::function<void(std::unique_ptr<std::vector<std::unique_ptr<Transaction>>>)> commit)
      : id_(id), f_(f), total_num_(total_num), verifier_(verifier), 
      single_call_(single_call), broadcast_call_(broadcast_call), commit_(commit){
  
  LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num;
  limit_count_ = 2*f+1;
  proposer_num_ = 2;
  batch_size_ = 15;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_);
  execute_id_ = 1;
  start_ = 0;
  is_stop_ = false;

  order_manager_ = std::make_unique<OrderManager>();

  send_thread_ = std::thread(&Tusk::AsyncSend, this);
  commit_thread_ = std::thread(&Tusk::AsyncCommitLeaderVertex, this);
  execute_thread_ = std::thread(&Tusk::AsyncCommitCausalHistory, this);
  cert_thread_ = std::thread(&Tusk::AsyncProcessCert, this);
  global_stats_ = Stats::GetGlobalStats();
}

Tusk::~Tusk() {
  if(send_thread_.joinable()){
    send_thread_.join();
  }
  if(commit_thread_.joinable()){
    commit_thread_.join();
  }
  if(cert_thread_.joinable()){
    cert_thread_.join();
  }
}

void Tusk::Stop() {
  is_stop_ = true;
}

bool Tusk::IsStop() {
  return is_stop_;
}


int Tusk::GetLeader(int64_t r) {
  return r / 2 % total_num_ + 1;
}

void Tusk::AsyncSend() {
  uint64_t last_time = 0;
  int current_proposer = 0;
  while (!IsStop()) {
    auto txn_hash = txns_.Pop();
    if(txn_hash == nullptr){
      continue;
    }
  /*
    auto txn_hash = txns_[current_proposer].Pop();
    if(txn_hash == nullptr){
      continue;

    }
    */
    // std::string prio_hash = GddTxnFromProposer();
    // assert(prio_hash!="");
    // *txn_hash = prio_hash;

    while(!IsStop()){
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return proposal_manager_->Ready(); });

      if(proposal_manager_->Ready()){
        start_= 1;
        break;
      }
    }
    assert(*txn_hash != "");

    int64_t queuing_time = proposal_manager_->SetQueuingTime(*txn_hash);

    //txn->set_queuing_time(GetCurrentTime() - txn->create_time());

    global_stats_->AddQueuingLatency(queuing_time);
    global_stats_->AddRoundLatency(GetCurrentTime() - last_time);
    last_time = GetCurrentTime();
    std::vector<std::unique_ptr<std::string> > txns;
    txns.push_back(std::move(txn_hash));
    for(int i = 1; ; ++i){
      auto txn_hash = txns_.Pop();
      if(txn_hash == nullptr){
        //break;
        continue;
      }
    /*
      auto txn_hash = txns_[current_proposer].Pop(0);
      if(txn_hash == nullptr){
        //break;
        continue;
      }
      */

      // std::string prio_hash = GddTxnFromProposer();
      // assert(prio_hash!="");
      // LOG(ERROR) << "txn_hash: " << *txn_hash;
      // LOG(ERROR) << "prio_hash: " << prio_hash;
      // *txn_hash = prio_hash;

      int64_t queuing_time = proposal_manager_->SetQueuingTime(*txn_hash);
      global_stats_->AddQueuingLatency(queuing_time);
      txns.push_back(std::move(txn_hash));
      if(txns.size()>= batch_size_){
        break;
      }
    }
    
    if (faulty_test_) {
      if (id_ % 3 == 1 && id_ < 3 * faulty_replica_num_) {
        std::reverse(txns.begin(), txns.end());
      }
    }

    LOG(ERROR) << "txns.size(): " << txns.size();
    auto proposal = proposal_manager_ -> GenerateProposal(txns);
    //LOG(ERROR)<<"gen proposal block";
    broadcast_call_(MessageType::NewBlock, *proposal);

    std::string block_size;
    proposal->SerializeToString(&block_size);
    //global_stats_->AddBlockSize(block_size.size());

    //LOG(ERROR)<<"add local block";
    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound()-1;
    //LOG(ERROR)<<"bc txn:"<<txns.size()<<" round:"<<round;
    if (round > 1) {
      if (round % 2 == 0) {
        CommitRound(round - 2);
      } 
    }
    current_proposer = (current_proposer + 1)%proposer_num_;
  }
}

void Tusk::AsyncCommitLeaderVertex() {
  int last_committed_round = -2;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      //LOG(ERROR) << "execu timeout";
      continue;
    }

    int round = *round_or;
    //LOG(ERROR)<<"commit round:"<<round;

    int new_round = last_committed_round;
    for (int r = last_committed_round + 2; r <= round; r += 2) {
      int leader = GetLeader(r);
      const Proposal * req = nullptr;
      // while(!IsStop()){
      //   req = proposal_manager_->GetRequest(r, leader);
      //   // req:"<<(req==nullptr);
      //   if (req == nullptr) {
      //     //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" not exit";
      //     //usleep(1000);
      //     std::unique_lock<std::mutex> lk(mutex_);
      //     vote_cv_.wait_for(lk, std::chrono::microseconds(100),
      //         [&] { return true; });

      //     continue;
      //   }
      //   break;
      req = proposal_manager_->GetRequest(r, leader);
      if (req == nullptr) {
        continue;
      }
      int reference_num = proposal_manager_->GetReferenceNum(*req);
      //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" ref:"<<reference_num;
      if (reference_num < limit_count_) {
        continue;
      } else {
        // LOG(ERROR) << "go to commit r:" << r << " previous:" << last_committed_round;
        for (int j = new_round+2; j <= r; j += 2) {
          int pre_leader = GetLeader(j);

          auto pre_req = proposal_manager_->GetRequest(j, pre_leader);
          if (pre_req == nullptr) {
            continue;
          }
          int reference_num = proposal_manager_->GetReferenceNum(*pre_req);
          if (reference_num < limit_count_ && !PathExist(req, pre_req)) {
            continue;
          }

          CommitLeaderVertex(j, pre_leader);
        }
      }
      new_round = r;
    }
    last_committed_round = new_round;
  }
}

std::unique_ptr<Transaction> Tusk::FetchTxn(const std::string& hash){
  return proposal_manager_->FetchTxn(hash);
}

void Tusk::AsyncCommitCausalHistory() {
  while (!IsStop()) {
    std::unique_ptr<Proposal>  proposal = leader_vertex_queue_.Pop();
    if (proposal == nullptr) {
      //LOG(ERROR) << "execu timeout";
      continue;
    }

    int commit_round = proposal->header().round();
    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - proposal->queuing_time();
    //global_stats_->AddCommitQueuingLatency(waiting_time);
    std::map<int, std::vector<std::unique_ptr<Proposal>>> round_2_comitted_proposals;
    std::queue<std::unique_ptr<Proposal>>q;
    q.push(std::move(proposal));
  
    while(!q.empty()){
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();
      //LOG(ERROR)<<"check proposal round:"<<p->header().round()<<" proposer:"<<p->header().proposer_id();

      for(auto& link : p->header().strong_cert().cert()){
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p = proposal_manager_->FetchRequest(link_round, link_proposer);
        if(next_p == nullptr){
          //LOG(ERROR)<<"no data round:"<<link_round<<" link proposer:"<<link_proposer;
          continue;
        }
        q.push(std::move(next_p));
      }

      for(auto& link : p->header().weak_cert().cert()){
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p = proposal_manager_->FetchRequest(link_round, link_proposer);
        if(next_p == nullptr){
          //LOG(ERROR)<<"no data round:"<<link_round<<" link proposer:"<<link_proposer;
          continue;
        }
        q.push(std::move(next_p));
      }
      round_2_comitted_proposals[p->header().round()].push_back(std::move(p));
    }

    int num = 0;
    int pro = 0;
      std::unique_ptr<std::vector<std::unique_ptr<Transaction> >> committed_lo_txn_list = std::make_unique<std::vector<std::unique_ptr<Transaction> >>();
    std::set<std::pair<int, int> > v;
    for(auto& it: round_2_comitted_proposals){
      for(auto& p : it.second){
        // LOG(ERROR)<<"=============== commit proposal round :"
        // <<p->header().round()
        // <<" proposer:"<<p->header().proposer_id()
        // <<" transaction size:"<<p->transactions_size()
        // <<" commit time:"<<(GetCurrentTime() - p->header().create_time())
        // <<" create time:"<< p->header().create_time() 
        // <<" proposal round:"<<p->header().round();

        global_stats_->AddCommitLatency(commit_time - p->header().create_time());
        //global_stats_->AddCommitLatency(commit_time - p->header().create_time() - waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round - p->header().round());

        //std::unique_ptr<std::vector<std::unique_ptr<Transaction> >> txns = std::make_unique<std::vector<std::unique_ptr<Transaction> >>();
        for(TxnDigest& dg : *p->mutable_digest()){
          std::unique_ptr<Transaction > txn = std::make_unique<Transaction>();
          txn->set_hash(dg.hash());
          txn->set_proxy_id(dg.proposer());
          txn->set_proposer(p->header().proposer_id());
          txn->set_user_seq(dg.seq());
          txn->set_create_time(dg.create_time());
          txn->set_queuing_time(dg.queuing_time());
          txn->set_proposal_id(pro);
          // [DK] Dependency graphs are constructed leade vertex by leader vertex, not round by round
          txn->set_round(p->header().round());
          //LOG(ERROR)<<" commit txn proposer:"<<p->header().proposer_id()<<" round:"<<p->header().round();

          //global_stats_->AddCommitDelay(commit_time- txn->create_time() - txn->queuing_time());

          v.insert(std::make_pair(dg.proposer(), dg.seq()));
          committed_lo_txn_list->push_back(std::move(txn));
          num++;
        }
        //LOG(ERROR)<<" commit txn size:"<<committed_lo_txn_list->size();
        //(*committed_lo_txn_list)[0]->set_queuing_time(GetCurrentTime());
        pro++;
      }
    }
    LOG(ERROR)<<"commit proposals: "<<committed_lo_txn_list->size()<<" round: "<<commit_round<<" blocks: "<<pro;
    //commit_(*txns);
        (*committed_lo_txn_list)[0]->set_queuing_time(GetCurrentTime());
    int64_t commit_time_end = GetCurrentTime();
    global_stats_->AddCommitRuntime(commit_time_end-commit_time);
    commit_(std::move(committed_lo_txn_list));
    //global_stats_->AddCommitTxn(num);
    //global_stats_->AddCommitTxn(v.size());
    //global_stats_->AddCommitBlock(pro);
  }
}


void Tusk::CommitLeaderVertex(int round, int proposer) {
  //LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer;
  int64_t commit_time = GetCurrentTime();
  global_stats_->AddCommitInterval(commit_time - last_commit_time_);
  last_commit_time_ = commit_time;

  std::unique_ptr<Proposal> p = proposal_manager_->FetchRequest(round, proposer);
  if (p == nullptr) {
    LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer;
    assert(false);
  }
  p->set_queuing_time(GetCurrentTime());
  leader_vertex_queue_.Push(std::move(p));
}

void Tusk::CommitRound(int round) {
  commit_queue_.Push(std::make_unique<int>(round));
}

void Tusk::AddTxnFromProposer(const Transaction& txn){
    std::unique_lock<std::mutex> lk(prio_mutex_);
    int proposer = txn.proxy_id(); 
    std::string hash = txn.hash();
    int seq = proposer_seq_[proposer];
    proposer_seq_[proposer]++;
    prio_txns_.push(Node(seq, proposer, hash));
}

std::string Tusk::GddTxnFromProposer(){
    std::unique_lock<std::mutex> lk(prio_mutex_);
    if(prio_txns_.empty()){
      return "";
    }
    Node node = prio_txns_.top();
    prio_txns_.pop();
    //LOG(ERROR)<<" get node proposer:"<<node.proposer<<" seq:"<<node.seq;
    return node.hash;
}


bool Tusk::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //LOG(ERROR)<<"recv txn";
  txn->set_create_time(GetCurrentTime());
  txn->set_id(local_txn_id_++);
  std::string hash = txn->hash();
  int proposer = txn->proxy_id()%proposer_num_;
  //LOG(ERROR)<<" proposer::"<<txn->proxy_id()<<" to:"<<proposer;
  AddTxnFromProposer(*txn);

  auto proxy_id = txn->proxy_id();
  auto user_seq = txn->user_seq();

  proposal_manager_->AddTxn(hash, std::move(txn));

  {
    std::unique_lock<std::mutex> lk(lo_mutex_);
    order_manager_->AddLocalOrderingRecord(proxy_id, user_seq);
    txns_.Push(std::make_unique<std::string>(hash));
  }
  //txns_[proposer].Push(std::make_unique<std::string>(hash));
  if(start_== 0){
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Tusk::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
  //           << " round:" << proposal->header().round();

      std::unique_lock<std::mutex> lk(check_block_mutex_);
  {
   proposal->set_queuing_time(GetCurrentTime());
    //std::unique_lock<std::mutex> lk(check_block_mutex_);
    if(!CheckBlock(*proposal)){
      std::unique_lock<std::mutex> lk(future_block_mutex_);
      //LOG(ERROR)<<"add future block:"<<proposal->header().round()<<" sender:"<<proposal->header().proposer_id();
      future_block_[proposal->header().round()][proposal->hash()] = std::move(proposal);
      return false;
    }
  }
  return SendBlockAck(std::move(proposal));
}

void Tusk::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
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
    broadcast_call_(MessageType::Cert, cert);
  }
}

bool Tusk::VerifyCert(const Certificate& cert){ 
  if(cert.metadata_size() < 2*f_+1){
    LOG(ERROR)<<" not enough certificates";
    return false;
  }

  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for(auto& metadata: cert.metadata()){
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if(!valid){
      return false;
    }
    hash_num[metadata.hash()]++;
    if(hash_num[metadata.hash()]>=2*f_+1){
      vote_num = true;
    }
  }
  return vote_num;
}

bool Tusk::PathExist(const Proposal *req1, const Proposal *req2) {
  auto round = req1->header().round();
  auto round_2 = req2->header().round();
  auto sender_2 = req2->sender();
  std::map<int, std::set<int>> round_2_senders;
  round_2_senders[round].insert(req1->sender());
  while(round > round_2) {
    for (auto &sender : round_2_senders[round]) {
      auto req = proposal_manager_->GetRequest(round, sender);
      for(auto& link : req->header().strong_cert().cert()){
        if (link.proposer() == sender_2) {
          return true;
        } else {
          round_2_senders[round-1].insert(link.proposer());
        }
      }
      for(auto& link : req->header().weak_cert().cert()){
        if (link.proposer() == sender_2) {
          return true;
        } else if (link.round() > round_2) {
          round_2_senders[link.round()].insert(link.proposer());
        }
      }
    }
    round--;
  }
  return false;
}

bool Tusk::CheckBlock(const Proposal& p){
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
  //LOG(ERROR)<<" check cert round:"<<p.header().round()<<" proposer:"<<p.header().proposer_id()<<" strong link size:"<<p.header().strong_cert().cert_size()<<" weak link size:"<<p.header().weak_cert().cert_size();
  return true;
}

bool Tusk::CheckCert(const Certificate& cert){
  if(!proposal_manager_->CheckBlock(cert.hash())){
    //future_cert_[cert->round()][cert->hash()] = std::move(cert);
    return false;
  }
  return true;
}

bool Tusk::SendBlockAck(std::unique_ptr<Proposal> proposal) {
  
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
    //LOG(ERROR) << "send back ack to block round:" << proposal->header().round()<<" from " << proposal->header().proposer_id();
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
  single_call_(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}


void Tusk::CheckFutureBlock(int round){
  //LOG(ERROR)<<" check block round from cert:"<<round;
  std::map<std::string,std::unique_ptr<Proposal>> hashs;
  {
    std::unique_lock<std::mutex> lk(future_block_mutex_);
    if(future_block_.find(round) == future_block_.end()){
      return;
    }
    for(auto& it : future_block_[round]){
      if(CheckBlock(*it.second)){
        //LOG(ERROR)<<" add new block round:"<<it.second->header().round();
        hashs[it.first] = std::move(it.second);
      }
    }

    //LOG(ERROR)<<" check round:"<<round<<" hash size:"<<hashs.size();
    for(auto& it : hashs){
      future_block_[round].erase(future_block_[round].find(it.first));
    }

    if(future_block_[round].size() == 0){
      future_block_.erase(future_block_.find(round));
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
    SendBlockAck(std::move(it.second));
  }
}

void Tusk::CheckFutureCert(int round, const std::string& hash, int proposer) {
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


void Tusk::AsyncProcessCert(){
  while(!IsStop()){
    std::unique_ptr<Certificate> cert = cert_queue_.Pop();
    if(cert == nullptr){
      continue;
    }

    int round = cert->round();
    int cert_sender = cert->proposer();

    //LOG(ERROR)<<" add cert, round:"<<round<<" from:"<<cert_sender;
    std::unique_lock<std::mutex> clk(check_block_mutex_);
    proposal_manager_->AddCert(std::move(cert));
    {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.notify_all();
    }

    CheckFutureBlock(round+1);
  }
}

void Tusk::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if(!VerifyCert(*cert)){
    assert(1==0);
    return;
  }

  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time-start_time); 

  {
    std::unique_lock<std::mutex> lk(check_block_mutex_);
    if(!CheckCert(*cert)){
      //LOG(ERROR)<<" add future cert round:"<<cert->round()<<" proposer:"<<cert->proposer();
      std::unique_lock<std::mutex> lk(future_cert_mutex_);
      future_cert_[cert->round()][cert->hash()] = std::move(cert);
      return;
    }
  }

  cert_queue_.Push(std::move(cert));
}

}  // namespace tusk
}  // namespace resdb
