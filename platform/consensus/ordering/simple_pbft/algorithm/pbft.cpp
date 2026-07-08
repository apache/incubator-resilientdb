#include "platform/consensus/ordering/simple_pbft/algorithm/pbft.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace simple_pbft {

Pbft::Pbft(const ResDBConfig& config, int id, int f, int total_num, int batch_size, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), config_(config), verifier_(verifier) {

  LOG(ERROR) << "get proposal graph:"<<batch_size;
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
  commit_seq_ = 0;
  batch_size_ = batch_size;

  send_thread_ = std::thread(&Pbft::AsyncSend, this);
  commit_thread_ = std::thread(&Pbft::AsyncCommit, this);
  global_stats_ = Stats::GetGlobalStats();
}

Pbft::~Pbft() {
  is_stop_ = true;
}

bool Pbft::IsStop() { return is_stop_; }

void Pbft::SetFailFunc(std::function<void(const Transaction& txn)> func) {
  fail_func_ = func;
}

void Pbft::SendFail(const Transaction& txn) {
  fail_func_(txn);
}

void Pbft::AsyncSend() {
  int water_mark = config_.GetConfigData().water_mark();
  int64_t start_time = GetCurrentTime();
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }
    //LOG(ERROR)<<" seq:"<<seq_<<" commit seq:"<<commit_seq_;
    if(seq_ - commit_seq_ > water_mark) {
      txns_.Push(std::move(txn));
      int64_t end_time = GetCurrentTime();
      //LOG(ERROR)<<" seq:"<<seq_<<" commit seq:"<<commit_seq_<<" gap fail"<<" del:"<<end_time - start_time;
      if(end_time - start_time > 2 && commit_seq_ > 0) {
        water_mark<<=1;
      }
      //SendFail(*txn);
      continue;
    }
    
    start_time = GetCurrentTime();
    //std::vector<std::unique_ptr<Transaction>> txns;
    //txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto next_txn = txns_.Pop(100);
      if (next_txn == nullptr) {
        break;
      }
      //txns.push_back(std::move(next_txn));
      *txn->add_subtxn()=*next_txn;
    }


    txn->set_create_time(GetCurrentTime());
    txn->set_seq(seq_++);
    txn->set_proposer(id_);

    //LOG(ERROR)<<"proposal === "<<batch_size_<<" time:"<<GetCurrentTime();
    Broadcast(MessageType::Propose, *txn);

  }
}

void Pbft::AsyncCommit() {
  int exq_txn = 1;
  while (!IsStop()) {
    auto proposal = commit_q_.Pop();
    if(proposal == nullptr) {
      continue;
    }
    int64_t seq = proposal->seq();

    std::unique_ptr<Transaction> txn = nullptr;
    while(txn == nullptr) {
      std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
      auto it = data_[seq%1000].find(proposal->hash());
      if(it != data_[seq%1000].end()){
        txn = std::move(it->second);
        data_[seq%1000].erase(it);
      }
    }
    assert(txn != nullptr);
    int txn_size = txn->subtxn_size();
    for(auto& subtx : *txn->mutable_subtxn()) {
      subtx.set_seq(exq_txn++);
      commit_(subtx);
    }
    txn->clear_subtxn();
    txn->set_seq(exq_txn++);
    global_stats_->AddLatency(GetCurrentTime() - txn->create_time());
    commit_(*txn);
    //LOG(ERROR)<<"receive commit:"<<" seq:"<<seq<<" exq txn:"<<exq_txn<<" txn size:"<<txn_size;
  }
}


bool Pbft::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  /*
  {
    txns_.Push(std::move(txn));
  }
  return true; 
  */

  int water_mark = config_.GetConfigData().water_mark();
  //LOG(ERROR)<<" get water mark:"<<water_mark;
  if(water_mark >0) {
    if(seq_ - commit_seq_ > water_mark) {
      //LOG(ERROR)<<" seq:"<<seq_<<" commit seq:"<<commit_seq_<<" gap fail";
      return false;
    }
  }

  txn->set_create_time(GetCurrentTime());
  txn->set_seq(seq_++);
  txn->set_proposer(id_);

  Broadcast(MessageType::Propose, *txn);
  
  return true;
}

bool Pbft::ReceivePropose(std::unique_ptr<Transaction> txn) {
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  int proposer = txn->proposer();
  uint64_t create_time = txn->create_time();
  //LOG(ERROR)<<"recv proposal from:"<<proposer<<" id:"<<seq<<" delay:"<<GetCurrentTime()-create_time;
  global_stats_->IncPropose();
  {
    // LOG(ERROR)<<"recv proposal";
    //LOG(ERROR)<<"recv txn from:"<<txn->proposer()<<" id:"<<txn->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    data_[seq%1000][txn->hash()]=std::move(txn);
  }

  Proposal proposal;
  proposal.set_hash(hash);
  proposal.set_seq(seq);
  proposal.set_proposer(id_);
  proposal.set_create_time(create_time);
  //global_stats_->AddLatency(GetCurrentTime() - create_time);

  Broadcast(MessageType::Prepare, proposal);
  return true;
}


bool Pbft::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  global_stats_->IncPrepare();
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  uint64_t create_time = proposal->create_time();
  //LOG(ERROR)<<"recv prepare from:"<<proposal->proposer()<<" id:"<<proposal->seq();
  //assert(proposer != 2);
  //assert(proposer != 3);
  std::unique_ptr<Transaction> txn = nullptr;
  bool done = false;
  {
    //LOG(ERROR)<<"recv proposal from:"<<proposal->proposer()<<" id:"<<proposal->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    received_[seq%1000][seq].insert(proposal->proposer());
    //received_[proposal->hash()].insert(proposal->proposer());

    if(received_[seq%1000][seq].size()==2*f_+1){
      done = true;
    }
  }
  if(done){
    Proposal proposal;
    proposal.set_hash(hash);
    proposal.set_seq(seq);
    proposal.set_proposer(id_);
    proposal.set_create_time(create_time);
    Broadcast(MessageType::Commit, proposal);
  }
  return true;
}

bool Pbft::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  uint32_t create_time = proposal->create_time();
  //std::unique_lock<std::mutex> lk(commit_mutex_[seq%1]);
  std::unique_lock<std::mutex> lk(commit_mutex_[seq%1]);
  commit_received_[seq%1][seq].insert(proposer);
  //LOG(ERROR)<<"receive commit:"<<proposer<<" num:"<<commit_received_[seq%1].size()<<" seq:"<<seq;
  //assert(proposer != 2);
  //assert(proposer != 3);
  if(commit_received_[seq%1][seq].size()==2*f_+1){
    //LOG(ERROR)<<" commit seq:"<<seq<<" commit seq:"<<commit_seq_;
    commit_q_.Push(std::move(proposal));
    commit_seq_++;
  }
  return true;

}

}  // namespace simple_pbft
}  // namespace resdb
