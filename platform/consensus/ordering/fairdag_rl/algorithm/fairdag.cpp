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
    graph_ = std::make_unique<Graph>();
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
  LOG(ERROR)<<" commit txn size:"<<txns.size();
  std::vector<std::string> commit_txns;
  std::map<std::string, Transaction*> hash2txn;
  for(int i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }
    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    commit_proposers_[key].insert(txns[i]->proposer());
    //LOG(ERROR)<<" proposal from:"<<txns[i]->proxy_id()<<" seq:"<<txns[i]->user_seq()<<" commit num:"<<commit_proposers_[key].size();
    if(commit_proposers_[key].size() >= f_+1){
      if(hash2txn.find(txns[i]->hash()) == hash2txn.end()){
        commit_txns.push_back(txns[i]->hash()); 
        //LOG(ERROR)<<" check proposal from:"<<txns[i]->proxy_id()<<" seq:"<<txns[i]->user_seq()<<" commit num:"<<commit_proposers_[key].size();
        hash2txn[txns[i]->hash()] = txns[i].get();
      }
    }

    for(int j = i+1; j < txns.size(); ++j){
      if(IsCommitted(*txns[j])){
        continue;
      }
      //LOG(ERROR)<<" add txn i:"<<i<<" j:"<<j;
      graph_->AddTxn(*txns[i], *txns[j]);
    }
  }

  LOG(ERROR)<<" commit txn size:"<<commit_txns.size();
  if(commit_txns.empty()){
    return;
  }
  std::vector<std::string> orders = graph_->GetOrder(commit_txns);
  LOG(ERROR)<<" orders size:"<<orders.size();
  std::vector<Transaction*> res;
  for(const std::string& hash : orders){
    LOG(ERROR)<<" get order:"<<hash2txn[hash]->proxy_id()<<" seq:"<<hash2txn[hash]->user_seq();
    res.push_back(hash2txn[hash]);
  }

  int last = orders.size()-1;
  for(; last>=0;--last){
    std::pair<int,int64_t> key = std::make_pair(res[last]->proxy_id(), res[last]->user_seq());
    if(commit_proposers_[key].size()>=2*f_+1){
      break;
    }
  }
  LOG(ERROR)<<" obtain order size:"<<last;

  if(last<0){
    return;
  }

  for(int i = 0; i < last; ++i){
    LOG(ERROR)<<" i:"<<i<<" last:"<<last;
    LOG(ERROR)<<" i:"<<i<<" last:"<<last<<" "<<res[i]->proxy_id()<<" seq:"<<res[i]->user_seq();;
    assert(res[i] != nullptr);
    LOG(ERROR)<<"remove";
    graph_->RemoveTxn(res[i]->hash());
    LOG(ERROR)<<"remove done";
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(res[i]->hash());
    if(data == nullptr){
      LOG(ERROR)<<"no txn from:"<<res[i]->proxy_id()<<" user seq:"<<res[i]->user_seq();
    }
    assert(data != nullptr);
    data->set_id(execute_id_++);
    LOG(ERROR)<<" commit:"<<res[last]->proxy_id()<<" seq:"<<res[last]->user_seq();
    SetCommitted(*data);
    Commit(*data);
  }
  return;
}




}  // namespace tusk
}  // namespace resdb
