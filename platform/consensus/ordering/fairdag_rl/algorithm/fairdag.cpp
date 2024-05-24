#include "platform/consensus/ordering/fairdag_rl/algorithm/fairdag.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag_rl {

FairDAG::FairDAG(
      int id, int f, int total_num, SignatureVerifier* verifier)
      : ProtocolBase(id, f, total_num), replica_num_(total_num){
  
   LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<replica_num_;

  tusk_ = std::make_unique<Tusk>(id, f, total_num, verifier, 
    [&](int type, const google::protobuf::Message& msg, int node){
    return SendMessage(type, msg, node);
  },
    [&](int type, const google::protobuf::Message& msg){
      return Broadcast(type, msg);
    },
    [&](std::vector<std::unique_ptr<Transaction>>& txns){
      return CommitTxns(txns);
    });
  local_time_ = 0;
  execute_id_ = 1;
  global_stats_ = Stats::GetGlobalStats();
}

FairDAG::~FairDAG() {
}

bool FairDAG::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
    std::unique_lock<std::mutex> lk(mutex_);
  //txn->set_timestamp(local_time_++);
  //txn->set_proposer(id_);
  //LOG(ERROR)<<"recv txn from:"<<txn->proxy_id()<<" proxy user seq:"<<txn->user_seq();
  return tusk_->ReceiveTransaction(std::move(txn));
}

bool FairDAG::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  return tusk_->ReceiveBlock(std::move(proposal));
}

void FairDAG::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  tusk_->ReceiveBlockACK(std::move(metadata));
}

void FairDAG::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  tusk_->ReceiveBlockCert(std::move(cert));
}

bool FairDAG::IsCommitted(const Transaction& txn){
  std::pair<int,int64_t> key = std::make_pair(txn.proxy_id(), txn.user_seq());
  return committed_.find(key) != committed_.end();
}

void FairDAG::AddTxnData(std::unique_ptr<Transaction> txn) {
  std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
  if(data_.find(key) == data_.end()){
    data_[key] = std::move(txn);
  }
}

void FairDAG::CommitTxns(std::vector<std::unique_ptr<Transaction> >& txns) {
  int64_t start_time = GetCurrentTime();
  std::vector<std::unique_ptr<Transaction>> execute_txns;
  int64_t commit_time = GetCurrentTime();
  for(auto& txna : txns){
    if(IsCommitted(*txna)){
      continue;
    }
    for(auto& txnb : txns) {
      if(IsCommitted(*txnb)){
        continue;
      }
      graph_->AddTxn(*txna, *txnb);
    }
  }
  for(auto& txn : txns){
    AddTxnData(std::move(txn));
  }
}




}  // namespace tusk
}  // namespace resdb
