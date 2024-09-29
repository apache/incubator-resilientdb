#include "platform/consensus/ordering/simple_pbft/algorithm/pbft.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace simple_pbft {

Pbft::Pbft(const ResDBConfig& config, int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), config_(config), verifier_(verifier) {

  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 0;
  commit_seq_ = 0;

  send_thread_ = std::thread(&Pbft::AsyncSend, this);
  commit_thread_ = std::thread(&Pbft::AsyncCommit, this);
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
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if(txn == nullptr){
      continue;
    }
    if(seq_ - commit_seq_ > 2048) {
      LOG(ERROR)<<" seq:"<<seq_<<" commit seq:"<<commit_seq_<<" gap fail";
      SendFail(*txn);
      continue;
    }

    txn->set_create_time(GetCurrentTime());
    txn->set_seq(seq_++);
    txn->set_proposer(id_);

    Broadcast(MessageType::Propose, *txn);

  }
}

void Pbft::AsyncCommit() {
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
    commit_(*txn);
    //LOG(ERROR)<<"receive commit:"<<" seq:"<<seq;
  }
}


bool Pbft::ReceiveTransaction(std::unique_ptr<Transaction> txn) {

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
  //LOG(ERROR)<<"recv proposal from:"<<proposer<<" id:"<<seq;
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

  Broadcast(MessageType::Prepare, proposal);
  return true;
}


bool Pbft::ReceivePrepare(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
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
    Broadcast(MessageType::Commit, proposal);
  }
  return true;
}

bool Pbft::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  std::string hash = proposal->hash();
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  //std::unique_lock<std::mutex> lk(commit_mutex_[seq%1]);
  std::unique_lock<std::mutex> lk(commit_mutex_[seq%1]);
  commit_received_[seq%1][seq].insert(proposer);
  //LOG(ERROR)<<"receive commit:"<<proposer<<" num:"<<commit_received_[seq%1].size()<<" seq:"<<seq;
  //assert(proposer != 2);
  //assert(proposer != 3);
  if(commit_received_[seq%1][seq].size()==2*f_+1){
    //LOG(ERROR)<<" commit:";
    commit_q_.Push(std::move(proposal));
    commit_seq_++;
  }
  return true;

}

}  // namespace simple_pbft
}  // namespace resdb
