#include "platform/consensus/ordering/fairdag/algorithm/tusk.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag {

Tusk::Tusk(int id, int f, int total_num, SignatureVerifier* verifier,
          protocol::ProtocolBase::SingleCallFuncType single_call,  
          protocol::ProtocolBase :: BroadcastCallFuncType broadcast_call,
          std::function<void(std::vector<std::unique_ptr<Transaction>> txns)> commit)
      : id_(id), f_(f), verifier_(verifier), total_num_(total_num), 
      single_call_(single_call), broadcast_call_(broadcast_call), commit_(commit){
  
  LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<total_num;
  limit_count_ = 2*f+1;
  batch_size_ = 200;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_);
  //next_seq_ = 1;
  local_txn_id_ = 1;
  execute_id_ = 1;
  start_ = 0;
  is_stop_ = false;

  send_thread_ = std::thread(&Tusk::AsyncSend, this);
  commit_thread_ = std::thread(&Tusk::AsyncCommit, this);
  execute_thread_ = std::thread(&Tusk::AsyncExecute, this);
}

Tusk::~Tusk() {
  if(send_thread_.joinable()){
    send_thread_.join();
  }
  if(commit_thread_.joinable()){
    commit_thread_.join();
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
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }

    while(!IsStop()){
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
          [&] { return proposal_manager_->Ready(); });

      if(proposal_manager_->Ready()){
        start_= 1;
        break;
      }
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    txns.push_back(std::move(txn));
    for(int i = 1; i < batch_size_; ++i){
      auto txn = txns_.Pop(0);
      if(txn == nullptr){
        break;
      }
      //LOG(ERROR)<<"propose txn from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq();
      txns.push_back(std::move(txn));
    }

    auto proposal = proposal_manager_ -> GenerateProposal(txns);
    //LOG(ERROR)<<"gen proposal block";
    broadcast_call_(MessageType::NewBlock, *proposal);
    //LOG(ERROR)<<"add local block";
    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound()-1;
    //LOG(ERROR)<<"bc txn:"<<txns.size()<<" round:"<<round;
    if (round > 1) {
      if (round % 2 == 0) {
        CommitRound(round - 2);
      } 
    }
  }
}

void Tusk::AsyncCommit() {
  int previous_round = -2;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      //LOG(ERROR) << "execu timeout";
      continue;
    }

    int round = *round_or;
    LOG(ERROR)<<"commit round:"<<round;

    int new_round = previous_round;
    for (int r = previous_round + 2; r <= round; r += 2) {
      int leader = GetLeader(r);
      auto req = proposal_manager_->GetRequest(r, leader);
      //LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" req:"<<(req==nullptr);
      if (req == nullptr) {
        continue;
      }
      int reference_num = proposal_manager_->GetReferenceNum(*req);
      LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" ref:"<<reference_num;
      if (reference_num < limit_count_) {
        continue;
      } else {
        // LOG(ERROR) << "go to commit r:" << r << " previous:" <<
        // previous_round;
        for (int j = new_round+2; j <= r; j += 2) {
          int leader = GetLeader(j);
          CommitProposal(j, leader);
        }
      }
      new_round = r;
    }
    previous_round = new_round;
  }
}

void Tusk::AsyncExecute() {
  while (!IsStop()) {
    std::unique_ptr<Proposal>  proposal = execute_queue_.Pop();
    if (proposal == nullptr) {
      //LOG(ERROR) << "execu timeout";
      continue;
    }

    std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
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
      ps[p->header().round()].push_back(std::move(p));
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    for(auto& it: ps){
      for(auto& p : it.second){
        LOG(ERROR)<<"=============== commit proposal round :"
        <<p->header().round()
        <<" proposer:"<<p->header().proposer_id()
        <<" transaction size:"<<p->transactions_size()
        <<" commit time:"<<(GetCurrentTime() - p->header().create_time())
        <<" create time:"<< p->header().create_time();
        for(auto& tx : *p->mutable_transactions()){
          txns.push_back(std::make_unique<Transaction>(tx));
        }
      }
    }
    LOG(ERROR)<<"commit proposal:"<<txns.size();
    commit_(std::move(txns));
  }
}


void Tusk::CommitProposal(int round, int proposer) {
  //LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer;
  std::unique_ptr<Proposal> p = proposal_manager_->FetchRequest(round, proposer);
  execute_queue_.Push(std::move(p));
}

void Tusk::CommitRound(int round) {
  commit_queue_.Push(std::make_unique<int>(round));
}

bool Tusk::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
//  LOG(ERROR)<<"recv txn";
  txn->set_id(local_txn_id_++);
  txns_.Push(std::move(txn));
  if(start_== 0){
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Tusk::VerifyCert(const Certificate& cert){ 
  if(cert.metadata_size() < 2*f_+1){
    LOG(ERROR)<<" not enough certificates";
    return false;
  }

  std::string data_str = cert.hash();

  int valid_num = 0;
  for(const Metadata& metadata : cert.metadata()){
    bool valid = verifier_->VerifyMessage(data_str, metadata.sign());
    if(!valid){
      LOG(ERROR) << "Verify message fail";
    }
    else {
      valid_num++;
    }
  }
  //LOG(ERROR)<<" valid num:"<<valid_num;
  return valid_num>=2*f_+1;
}

bool Tusk::ReceiveBlock(std::unique_ptr<Proposal> proposal) {

  LOG(ERROR)<<"recv block from "<<proposal->header().proposer_id()<<" round:"<<proposal->header().round();
  Metadata metadata;

  Metadata::Data * data = metadata.mutable_data();
  data->set_hash(proposal->hash());
  data->set_round(proposal->header().round());
  data->set_proposer(proposal->header().proposer_id());

/*
  std::string time_data;
  for(const auto& txn: proposal->transactions()){
    time_data += std::to_string(txn.timestamp());
  }
  */

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
    proposal_manager_->AddBlock(std::move(proposal));
  }
  single_call_(MessageType::BlockACK, metadata, metadata.data().proposer());
  return true;
}

void Tusk::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->data().hash();
  int round = metadata->data().round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  LOG(ERROR)<<"recv block ack from:"<<sender<<" num:"<<received_num_[hash].size();
  if(received_num_[hash].size() == limit_count_){
    Certificate cert;
    for(auto& it : received_num_[hash]){
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    const Proposal * p = proposal_manager_->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    *cert.mutable_strong_cert() = p->header().strong_cert();

    //LOG(ERROR)<<"send cert:";
    broadcast_call_(MessageType::Cert, cert);
  }
}

void Tusk::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  if(!VerifyCert(*cert)){
    return;
  }
  //LOG(ERROR)<<"recv cert:"<<cert->proposer();
  proposal_manager_->AddCert(std::move(cert));
  //LOG(ERROR)<<"add cert done";
  std::unique_lock<std::mutex> lk(mutex_);
  vote_cv_.notify_all();
}


}  // namespace tusk
}  // namespace resdb
