#include "platform/consensus/ordering/zzy/algorithm/zzy.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace zzy {

ZZY::ZZY(const ResDBConfig& config, int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), config_(config), verifier_(verifier) {

  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  seq_ = 1;
  commit_seq_ = 0;

  commit_thread_ = std::thread(&ZZY::AsyncCommit, this);
}

ZZY::~ZZY() {
  is_stop_ = true;
}

bool ZZY::IsStop() { return is_stop_; }

void ZZY::SetFailFunc(std::function<void(const Transaction& txn)> func) {
  fail_func_ = func;
}

void ZZY::SendFail(const Transaction& txn) {
  fail_func_(txn);
}

void ZZY::AsyncCommit() {
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
    {
      std::unique_lock<std::mutex> lk(commit_mutex_[seq%1000]);
      committed_[seq%1000].insert(txn->hash());
      //LOG(ERROR)<<"receive commit:"<<" seq:"<<seq<<" txn hash:"<<txn->hash().size();
    }
  }
}


bool ZZY::ReceiveTransaction(std::unique_ptr<Transaction> txn) {

  int water_mark = config_.GetConfigData().water_mark();
  //LOG(ERROR)<<" get water mark:"<<water_mark<<" seq:"<<seq_<<" commit :"<<commit_seq_;
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

bool ZZY::ReceivePropose(std::unique_ptr<Transaction> txn) {
  std::string hash = txn->hash();
  int64_t seq = txn->seq();
  int proposer = txn->proposer();
  int client = txn->proxy_id();
  //LOG(ERROR)<<"recv proposal from:"<<proposer<<" id:"<<seq<<" client:"<<client;
  {
    // LOG(ERROR)<<"recv proposal";
    //LOG(ERROR)<<"recv txn from:"<<txn->proposer()<<" id:"<<txn->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    data_[seq%1000][txn->hash()]=std::move(txn);
  }

  {
    std::unique_ptr<Proposal> comit_proposal = std::make_unique<Proposal>();
    comit_proposal->set_hash(hash);
    comit_proposal->set_seq(seq);
    commit_q_.Push(std::move(comit_proposal));
    commit_seq_++;
  }

  return true;
}

bool ZZY::ReceiveACK(std::unique_ptr<Proposal> proposal) {
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  //LOG(ERROR)<<" client receive seq:"<<seq<<" proposer:"<<proposer;
  return true;
}

bool ZZY::Verify(const Proposal& proposal) {

/*
  if(proposal.qc().qc_size() < 2*f_+1) {
    LOG(ERROR)<<" qc not enough:"<<proposal.qc().qc_size();
    assert(proposal.qc().qc_size() >= 2*f_+1);
    return false;
  }
  */

  for(const auto& qc : proposal.qc().qc()) {
    std::string data_hash = qc.data_hash();

    bool valid =
        verifier_->VerifyMessage(data_hash, qc.sign());
    if (!valid) {
      LOG(ERROR)<<" sign not valid, hash size:"<<data_hash.size();
      return false;
    }
  }
  return true;
}

bool ZZY::ReceiveCommit(std::unique_ptr<Proposal> proposal) {
  int64_t seq = proposal->seq();
  int proposer = proposal->proposer();
  int64_t local_id = proposal->local_id();
  std::string hash = proposal->hash();
  //LOG(ERROR)<<" client commit seq:"<<seq<<" proposer:"<<proposer<<" hash size:"<<hash.size()<<" local_id:"<<local_id;
    assert(Verify(*proposal));

    std::unique_lock<std::mutex> lk(commit_mutex_[seq%1000]);
    if(committed_[seq%1000].find(hash) == committed_[seq%1000].end()){
      // LOG(ERROR)<<"recv proposal";
      //LOG(ERROR)<<"recv txn from:"<<txn->proposer()<<" id:"<<txn->seq();
      std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
      if(data_[seq%1000].find(hash) != data_[seq%1000].end()){
        {
          std::unique_ptr<Proposal> comit_proposal = std::make_unique<Proposal>();
          comit_proposal->set_hash(hash);
          comit_proposal->set_seq(seq);
          commit_q_.Push(std::move(comit_proposal));
          commit_seq_++;
        }
      }
    }
    else {
    assert(committed_[seq%1000].find(hash) != committed_[seq%1000].end());
    }

  /*
  {
    // LOG(ERROR)<<"recv proposal";
    //LOG(ERROR)<<"recv txn from:"<<txn->proposer()<<" id:"<<txn->seq();
    std::unique_lock<std::mutex> lk(mutex_[seq%1000]);
    if(data_[seq%1000].find(txn->hash()) != data_[seq%1000].end()){

      {
        std::unique_ptr<Proposal> comit_proposal = std::make_unique<Proposal>();
        comit_proposal->set_hash(hash);
        comit_proposal->set_seq(seq);
        commit_q_.Push(std::move(comit_proposal));
        commit_seq_++;
      }
    }
  }
  */


  {
    Proposal proposal;
    proposal.set_hash(hash);
    proposal.set_seq(seq);
    proposal.set_proposer(id_);
    proposal.set_local_id(local_id);

    SendMessage(MessageType::CommitACK, proposal, proposer);
  }

  return true;
}

}  // namespace zzy
}  // namespace resdb
