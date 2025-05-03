#include "platform/consensus/ordering/fairdag/algorithm/fairdag.h"

#include <glog/logging.h>
#include "common/utils/utils.h"


namespace resdb {
namespace fairdag {

FairDAG::FairDAG(
      int id, int f, int total_num, SignatureVerifier* verifier)
      : ProtocolBase(id, f, total_num), replica_num_(total_num){
  
   LOG(ERROR)<<"id:"<<id<<" f:"<<f<<" total:"<<replica_num_;
  min_timestamp_.resize(replica_num_+1);

  tusk_ = std::make_unique<Tusk>(id, f, total_num, verifier, 
    [&](int type, const google::protobuf::Message& msg, int node){
    return SendMessage(type, msg, node);
  },
    [&](int type, const google::protobuf::Message& msg){
      return Broadcast(type, msg);
    },
    [&](std::vector<std::unique_ptr<Transaction>>& txns){
      return OrderInCausalHistory(txns);
    },
    [&](const Proposal* p){
      return CollectTimestamp(p);
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

void FairDAG::SortTxn(std::vector<std::unique_ptr<Transaction>>& txns, 
  std::map<std::string, uint64_t>& assigned_time){

  auto cmp = [&](std::unique_ptr<Transaction>& txn1, std::unique_ptr<Transaction>&txn2) {
    return assigned_time[txn1->hash()] < assigned_time[txn2->hash()] ||
      (assigned_time[txn1->hash()] == assigned_time[txn2->hash()] 
      && txn1->proposer() < txn2->proposer());
  };

  sort(txns.begin(), txns.end(), cmp);
}

void FairDAG::CollectTimestamp(const Proposal* p) {
  //LOG(ERROR) << "[CollectTimestamp]: from" << p->header().proposer_id() << " round:" << p->header().round();
  auto proposer_id = p->header().proposer_id();
  std::unique_lock<std::mutex> lk(ts_mutex_);
  for(const TxnDigest& dg : p->digest()){
    std::pair<int,int64_t> key = std::make_pair(dg.proposer(), dg.seq());
    if(ready_.find(key) != ready_.end()){
      continue;
    }
    if(dg.ts() >= min_timestamp_[proposer_id]) {
      min_timestamp_[proposer_id] = dg.ts() + 1;
    }
    uncommitted_txns_[key].insert(std::make_pair(proposer_id, dg.ts()));
  }
}

uint64_t FairDAG::GetLowerboundTimestamp(const std::pair<int,int64_t>& key) {
    const auto& commit_timelist = committed_txns_[key];
    const auto& uncommit_timelist = uncommitted_txns_[key];
    std::vector<int64_t> times;
    for(int i = 1; i <= replica_num_; ++i){
      auto it = commit_timelist.find(i);
      auto it2 = uncommit_timelist.find(i);
      //  LOG(ERROR) << "Y2 " << i;
      if(it != commit_timelist.end()){
        times.push_back(it->second);
    //LOG(ERROR)<<"commit time:"<<it->second;
      }
      else if (it2 != uncommit_timelist.end()) {
        times.push_back(it2->second);
      }
      else {
        times.push_back(min_timestamp_[i]);
    //LOG(ERROR)<<"uncommit time:"<<min_timestamp_[i];
      }
    }
    sort(times.begin(), times.end());
    assert(times.size()>f_);
    // LOG(ERROR)<<"get uncommit time lowbound:"<<times[f_];
    return times[f_];
}

uint64_t FairDAG::GetCommitTimestamp(const std::pair<int,int64_t>& key) {
  const auto& commit_timelist = committed_txns_[key];
  std::vector<uint64_t> times;
  for(auto it : commit_timelist){
      times.push_back(it.second);
  }
  //LOG(ERROR)<<" time size:"<<times.size();
  //assert(static_cast<int>(times.size())>=2*f_+1);
  sort(times.begin(), times.end());
  //LOG(ERROR)<<"assign commit time:"<<times[f_+1];
  return times[f_];
}

void FairDAG::OrderInCausalHistory(std::vector<std::unique_ptr<Transaction> >& txns_w_ts) {

/*
  LOG(ERROR)<<" comit txn:"<<txns_w_ts.size();
  for(auto& txn: txns_w_ts){
    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    if(committed_.find(key) != committed_.end()){
      continue;
    }
    committed_.insert(key);
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(txn->hash());
    if(data == nullptr){
      //LOG(ERROR)<<"no txn from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq();
      continue;
    }
    assert(data != nullptr);
    data->set_id(execute_id_++);
    Commit(*data);
  }
  return;
  */

  //LOG(ERROR) << "XXX1";
  std::unique_lock<std::mutex> lk(ts_mutex_);
  int64_t start_time = GetCurrentTime();
  std::vector<std::unique_ptr<Transaction>> execute_txns;
  int64_t commit_time = GetCurrentTime();
  for(auto& txn : txns_w_ts){
    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    // [ISSUE][DK] we may have a better way to calculate the lowest bound for each transaction by adding uncommitted_txns_
    committed_txns_[key].insert(std::make_pair(txn->proposer(), txn->timestamp()));
  //LOG(ERROR) << "XXX1";
    if(uncommitted_txns_[key].find(txn->proposer()) != uncommitted_txns_[key].end()) {
      uncommitted_txns_[key].erase(uncommitted_txns_[key].find(txn->proposer()));
    }
  //LOG(ERROR) << "XXX1";
    if(committed_time_.find(key) == committed_time_.end()){
      committed_time_[key] = commit_time;
    }
    // // [ISSUE][DK] may be a bit inefficient, it is better to calculate it when committing the transactions
    // min_timestamp_[txn->proposer()] = std::max(min_timestamp_[txn->proposer()], txn->timestamp()+1);
    //LOG(ERROR)<<"proposer:"<<txn->proposer()<<" next commit time:"<<min_timestamp_[txn->proposer()]<<" timestamp:"<<txn->timestamp();
  }
  //LOG(ERROR) << "XXX2";
  int num = 0;
  uint64_t lower_bound_uncommtted_time = -1;
  std::map<std::string, uint64_t> exec_assigned_ts;

  // Divide txns based on if they are ready to be ordered and executed
  for(auto& txn : txns_w_ts){
    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    if(ready_.find(key) != ready_.end()){
      continue;
    }
    // LOG(ERROR)<<"commit from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq()<<" time num:"<<committed_txns_[key].size()<<" proposer:"<<txn->proposer()<<" timestamp:"<<txn->timestamp()<<" lowerbound:"<<lower_bound_uncommtted_time<<" commit time:"<<commit_time<<" first time:"<<committed_time_[key];

    if(static_cast<int>(committed_txns_[key].size()) < 2*f_+1){
      // LOG(ERROR) << "XXX3";
      uint64_t lower_bound_time = GetLowerboundTimestamp(key);
      lower_bound_uncommtted_time = std::min(lower_bound_uncommtted_time, lower_bound_time);
    }
    else {
      // LOG(ERROR) << "XXX4";
      if (assigned_ts_.find(key) == assigned_ts_.end()){
        int64_t assigned_time  = GetCommitTimestamp(key);
        assigned_ts_[key] = assigned_time;
        // [ISSUE][DK] The lower_bound_uncommtted_time is becoming lower and lower, it is possible that some execute_txns should be removed; fixed in line 176-190
        if(assigned_time < lower_bound_uncommtted_time){
	LOG(ERROR)<<"wait delay:"<<(GetCurrentTime() - committed_time_[key]);

            exec_assigned_ts[txn->hash()] = assigned_time;
            execute_txns.push_back(std::move(txn));
            num++;
        } else {
          not_ready_.insert(std::make_pair(assigned_time, std::move(txn)));
        }
      }
    }
  }
   //LOG(ERROR) << "XXX5";
  {
    int j = 0;
    int size = execute_txns.size();
    for(int i = 0; i < size; i++){
      auto ats = exec_assigned_ts[execute_txns[i]->hash()];
      if(ats < lower_bound_uncommtted_time) {
        execute_txns[j++] = std::move(execute_txns[i]);
      } else {
        not_ready_.insert(std::make_pair(ats, std::move(execute_txns[i])));
      }
    }
    while (j++ < size) {
      execute_txns.pop_back();
    }
  }

  // LOG(ERROR)<<" lower_bound uncommitted time:"<<lower_bound_uncommtted_time;
  // [DK] Add the transactions in previous causal histories that were not_ready but are ready now
  for(auto it = not_ready_.begin(); it != not_ready_.end();){
    if(it->first <lower_bound_uncommtted_time){
        //std::pair<int,int64_t> key = std::make_pair(it->second->proxy_id(), it->second->user_seq());
        //LOG(ERROR)<<" get from pending: proposer:"<<it->second->proposer()<<" assign time:"<<it->first<<" user seq:"<<it->second->user_seq();
        exec_assigned_ts[it->second->hash()] = it->first;
        execute_txns.push_back(std::move(it->second));
        auto next = it;
        next++;
        not_ready_.erase(it);
        it = next;
    }
    else {
      break;
    }
  }

  //LOG(ERROR)<<" assign time:"<<exec_assigned_ts.size();

  SortTxn(execute_txns, exec_assigned_ts);
  int txn_nums = 0;
  for(auto& txn: execute_txns){

    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    LOG(ERROR)<<"commit from proposer:"<<txn->proxy_id()<<" time:"<<txn->timestamp()<<" seq:"<<txn->user_seq()<<" proposer:"<<txn->proposer()<<" commit time:"<<commit_time<<" first time:"<<committed_time_[key]<<" delay:"<<(GetCurrentTime() - committed_time_[key]);
    
    global_stats_->AddExecuteDelay(GetCurrentTime() - committed_time_[key]);

    assert(ready_.find(key) == ready_.end());
    ready_.insert(key);
    
    committed_txns_[key].clear();
    committed_txns_.erase(key);
    uncommitted_txns_[key].clear();
    uncommitted_txns_.erase(key);

    //global_stats_->AddCommitDelay(commit_time- txn->create_time() - txn->queuing_time());

      std::unique_ptr<Transaction> data = tusk_->FetchTxn(txn->hash());
      if(data == nullptr){
        LOG(ERROR)<<"no txn from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq();
      }
      assert(data != nullptr);
      data->set_id(execute_id_++);
      
      //LOG(ERROR)<<" data queuing time:"<<data->queuing_time()<<" txn queuing time:"<<txn->queuing_time();

      //global_stats_->AddCommitDelay(commit_time- data->create_time() - data->queuing_time());
      Commit(*data);
      txn_nums++;
  }
  int64_t end_time = GetCurrentTime();
  global_stats_->AddExecutePrepareDelay(end_time-start_time);

  double ratio = static_cast<double>(execute_txns.size())/ txns_w_ts.size();
  global_stats_->AddCommitRatio(ratio*100000);
  //LOG(ERROR)<<"txn_nums:"<<txn_nums;
  //LOG(ERROR)<<"exe time:"<<execute_txns.size()<<" total:"<<txns.size()<<" ratio:"<<ratio<<" txn_nums:"<<txn_nums;

  return ;
}


}  // namespace tusk
}  // namespace resdb
