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
  //LOG(ERROR)<<" commit txn size:"<<txns.size();
  std::vector<int> commit_txns;
  std::map<int, Transaction*> hash2txn;
  std::vector<std::pair<int,int>> pendings;
  for(int i = 0; i < txns.size(); ++i){
    if(IsCommitted(*txns[i])){
      continue;
    }

    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    if(commit_proposers_idx_.find(key) == commit_proposers_idx_.end()){
      commit_proposers_idx_[key]=idx_++;
    }
    int id = commit_proposers_idx_[key];

    //LOG(ERROR)<<" add pending id:"<<id<<" proxy:"<<txns[i]->proxy_id()<<" user seq:"<<txns[i]->user_seq();
    pendings.push_back(std::make_pair(i,id));
  }

  for(int k = 0; k <  pendings.size(); ++k){
    int i = pendings[k].first;
      int id1 = pendings[k].second;

    std::pair<int,int64_t> key = std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    commit_proposers_[key].insert(txns[i]->proposer());
    //LOG(ERROR)<<" proposal from:"<<txns[i]->proxy_id()<<" seq:"<<txns[i]->user_seq()<<" commit num:"<<commit_proposers_[key].size();
    if(commit_proposers_[key].size() >= f_+1){
      if(hash2txn.find(id1)== hash2txn.end()){
        commit_txns.push_back(id1); 
        //LOG(ERROR)<<" check proposal from:"<<txns[i]->proxy_id()<<" seq:"<<txns[i]->user_seq()<<" commit num:"<<commit_proposers_[key].size();
        hash2txn[id1] = txns[i].get();
      }
    }

    for(int kk = k+1; kk < pendings.size(); ++kk){
      int j = pendings[kk].first;
      assert(i<txns.size());
      assert(j<txns.size());

      int id2 = pendings[kk].second;
      if(id1==id2)continue;
      //LOG(ERROR)<<" add txn i:"<<i<<" j:"<<j<<" id1:"<<id1<<" id2:"<<id2;
      graph_->AddTxn(id1, id2);
      //graph_->AddTxn(*txns[i], *txns[j]);
    }
  }
  //LOG(ERROR)<<" add:"<<graph_->Size();
  if(commit_txns.empty()){
    return;
  }
  std::vector<int> orders = graph_->GetOrder(commit_txns);

  //LOG(ERROR)<<" orders size:"<<orders.size();
  std::vector<Transaction*> res;
  for(int id: orders){
     assert(hash2txn.find(id) != hash2txn.end());
    //LOG(ERROR)<<" get order:"<<hash2txn[id]->proxy_id()<<" seq:"<<hash2txn[id]->user_seq();
    res.push_back(hash2txn[id]);
  }

  int last = orders.size()-1;
  for(; last>=0;--last){
    std::pair<int,int64_t> key = std::make_pair(res[last]->proxy_id(), res[last]->user_seq());
    if(commit_proposers_[key].size()>=2*f_+1){
      break;
    }
  }

  //LOG(ERROR)<<" obtain order size:"<<last;
/*
  for(const auto& txn : txns){
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(txn->hash());
    if(data==nullptr)continue;
    assert(data != nullptr);
    data->set_id(execute_id_++);
    SetCommitted(*data);
    Commit(*data);
    //graph_->RemoveTxn(*txn);

    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    int id = commit_proposers_idx_[key];
    graph_->RemoveTxn(id);
  }

  //LOG(ERROR)<<" remove:"<<graph_->Size();
  return;
  */

  if(last<0){
    return;
  }

  for(int i = 0; i < last; ++i){
    //LOG(ERROR)<<" i:"<<i<<" last:"<<last;
    std::pair<int,int64_t> key = std::make_pair(res[i]->proxy_id(), res[i]->user_seq());
    int id = commit_proposers_idx_[key];
    //LOG(ERROR)<<" i:"<<i<<" last:"<<last<<" "<<res[i]->proxy_id()<<" seq:"<<res[i]->user_seq()<<" key id:"<<id;
    assert(res[i] != nullptr);
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(res[i]->hash());
    if(data == nullptr){
      LOG(ERROR)<<"no txn from:"<<res[i]->proxy_id()<<" user seq:"<<res[i]->user_seq();
    }
    assert(data != nullptr);
    data->set_id(execute_id_++);
    //LOG(ERROR)<<" commit:"<<res[last]->proxy_id()<<" seq:"<<res[last]->user_seq();
    SetCommitted(*data);
    Commit(*data);

    graph_->RemoveTxn(id);
  }
  return;
}




}  // namespace tusk
}  // namespace resdb
