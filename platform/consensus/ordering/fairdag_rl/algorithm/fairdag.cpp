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

void FairDAG::SetCommitted(const Transaction& txn){
  std::pair<int,int64_t> key = std::make_pair(txn.proxy_id(), txn.user_seq());
  committed_.insert(key);
}

void FairDAG::AddTxnData(std::unique_ptr<Transaction> txn) {
  std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
  if(data_.find(key) == data_.end()){
    data_[key] = std::move(txn);
  }
}

void FairDAG::CommitTxns(const std::vector<std::unique_ptr<Transaction> >& txns) {
  //int64_t start_time = GetCurrentTime();
  //int64_t commit_time = GetCurrentTime();

  std::vector<std::string> commit_txns;
  std::map<std::string, Transaction*> hash2txn;
  for(int i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    commit_proposers_[key].insert(txns[i]->proposer());
    if(commit_proposers_[key].size() >= f_+1){
      commit_txns.push_back(txns[i]->hash()); 
      hash2txn[txns[i]->hash()] = txns[i].get();
    }

    for(int j = i; j < txns.size(); ++j){
      if(IsCommitted(*txns[j])){
        continue;
      }
      graph_->AddTxn(*txns[i], *txns[j]);
    }
  }

  std::vector<std::string> orders = graph_->GetOrder(commit_txns);
  std::vector<Transaction*> res;
  for(const std::string& hash : orders){
    res.push_back(hash2txn[hash]);
  }

  int last = orders.size()-1;
  for(; last>=0;--last){
    std::pair<int,int64_t> key = std::make_pair(res[last]->proxy_id(), res[last]->user_seq());
    if(commit_proposers_[key].size()>=2*f_+1){
      break;
    }
  }

  if(last<0){
    return;
  }

  for(int i = 0; i < last; ++i){
    graph_->RemoveTxn(res[i]->hash());
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(res[i]->hash());
    if(data == nullptr){
      LOG(ERROR)<<"no txn from:"<<res[i]->proxy_id()<<" user seq:"<<res[i]->user_seq();
    }
    assert(data != nullptr);
    data->set_id(execute_id_++);
    SetCommitted(*data);
    Commit(*data);
  }
  return;
}




}  // namespace tusk
}  // namespace resdb
