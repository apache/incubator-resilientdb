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

void FairDAG::SortTxn(std::vector<std::unique_ptr<Transaction>>& txns, 
  std::map<std::string, uint64_t>& assigned_time){

  auto cmp = [&](std::unique_ptr<Transaction>& txn1, std::unique_ptr<Transaction>&txn2) {
    return assigned_time[txn1->hash()] < assigned_time[txn2->hash()] ||
      (assigned_time[txn1->hash()] == assigned_time[txn2->hash()] 
      && txn1->proposer() < txn2->proposer());
  };

  sort(txns.begin(), txns.end(), cmp);
}

uint64_t FairDAG::TryAssign(const std::pair<int,int64_t>& key) {
   return 0;
    const auto& commit_timelist = committed_txns_[key];
    if(static_cast<int>(commit_timelist.size()) < f_+1){
      return 0;
    }

    std::vector<uint64_t> times;
    for(const auto& it : commit_timelist){
      times.push_back(it.second);
      //LOG(ERROR)<<"get :"<<it.second;
    }
    assert(times.size()>=f_+1);

    sort(times.begin(), times.end());

    uint64_t max_t = times[f_];
  //LOG(ERROR)<<"try min:"<<max_t;
    for(int i = 1; i <= replica_num_; ++i){
    //LOG(ERROR)<<"min time:"<<i<<" min_timestamp_[i]:"<<min_timestamp_[i];
      if(min_timestamp_[i]<max_t){
        return 0;
      }
    }

    return max_t;
}

uint64_t FairDAG::GetLowerboundTimestamp(const std::pair<int,int64_t>& key) {
    const auto& commit_timelist = committed_txns_[key];
    std::vector<int64_t> times;
    for(int i = 1; i <= replica_num_; ++i){
      auto it = commit_timelist.find(i);
      if(it != commit_timelist.end()){
        times.push_back(it->second);
    //LOG(ERROR)<<"commit time:"<<it->second;
      }
      else {
        times.push_back(min_timestamp_[i]);
    //LOG(ERROR)<<"uncommit time:"<<min_timestamp_[i];
      }
    }
    sort(times.begin(), times.end());
    assert(times.size()>f_);
    //LOG(ERROR)<<"get uncommit time lowbound:"<<times[f_];
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

void FairDAG::CommitTxns(std::vector<std::unique_ptr<Transaction> >& txns) {
  int64_t start_time = GetCurrentTime();
  std::vector<std::unique_ptr<Transaction>> execute_txns;
  int64_t commit_time = GetCurrentTime();
  for(auto& txn : txns){
    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());

    committed_txns_[key].insert(std::make_pair(txn->proposer(), txn->timestamp()));
    if(committed_time_.find(key) == committed_time_.end()){
      committed_time_[key] = commit_time;
    }
    min_timestamp_[txn->proposer()] = std::max(min_timestamp_[txn->proposer()], txn->timestamp()+1);
    //LOG(ERROR)<<"proposer:"<<txn->proposer()<<" next commit time:"<<min_timestamp_[txn->proposer()]<<" timestamp:"<<txn->timestamp();
  }

  int num = 0;
  uint64_t lower_bound_uncommtted_time = -1;
  std::map<std::string, uint64_t> assign_time;
  for(auto& txn : txns){
    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());

    //LOG(ERROR)<<"commit from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq()<<" time num:"<<committed_txns_[key].size()<<" proposer:"<<txn->proposer()<<" timestamp:"<<txn->timestamp()<<" lowerbound:"<<lower_bound_uncommtted_time<<" commit time:"<<commit_time<<" first time:"<<committed_time_[key];

    if(static_cast<int>(committed_txns_[key].size()) < 2*f_+1){
      uint64_t lower_bound_time = GetLowerboundTimestamp(key);
      lower_bound_uncommtted_time = std::min(lower_bound_uncommtted_time, lower_bound_time);
    }
    else {
      if (assigned_time_.find(key) == assigned_time_.end()){
        int64_t assigned_time  = GetCommitTimestamp(key);
        assigned_time_[key] = assigned_time;

        if(assigned_time < lower_bound_uncommtted_time){
            assign_time[txn->hash()] = assigned_time;
            execute_txns.push_back(std::move(txn));
            num++;
        }
        else {
          not_ready_.insert(std::make_pair(assigned_time, std::move(txn)));
        }
      }
    }
  }

  //LOG(ERROR)<<" lower_bound uncommitted time:"<<lower_bound_uncommtted_time;
  for(auto it = not_ready_.begin(); it != not_ready_.end();){
    if(it->first <lower_bound_uncommtted_time){
        //std::pair<int,int64_t> key = std::make_pair(it->second->proxy_id(), it->second->user_seq());
        //LOG(ERROR)<<" get from pending: proposer:"<<it->second->proposer()<<" assign time:"<<it->first<<" user seq:"<<it->second->user_seq();
          assign_time[it->second->hash()] = it->first;
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

  //LOG(ERROR)<<" assign time:"<<assign_time.size();

  SortTxn(execute_txns, assign_time);

  for(auto& txn: execute_txns){

    std::pair<int,int64_t> key = std::make_pair(txn->proxy_id(), txn->user_seq());
    //LOG(ERROR)<<"commit from proposer:"<<txn->proxy_id()<<" time:"<<txn->timestamp()<<" seq:"<<txn->user_seq()<<" proposer:"<<txn->proposer()<<" commit time:"<<commit_time<<" first time:"<<committed_time_[key]<<" delay:"<<(GetCurrentTime() - committed_time_[key]);
    
    global_stats_->AddExecuteDelay(GetCurrentTime() - committed_time_[key]);

    if(ready_.find(key) != ready_.end()){
      continue;
    }
    //assert(ready_.find(key) == ready_.end());
    ready_.insert(key);
    //global_stats_->AddCommitDelay(commit_time- txn->create_time() - txn->queuing_time());

    while(true){
      std::unique_ptr<Transaction> data = tusk_->FetchTxn(txn->hash());
      if(data == nullptr){
        LOG(ERROR)<<"no txn from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq();
      }
      assert(data != nullptr);
      data->set_id(execute_id_++);
      //LOG(ERROR)<<" data queuing time:"<<data->queuing_time()<<" txn queuing time:"<<txn->queuing_time();

      //global_stats_->AddCommitDelay(commit_time- data->create_time() - data->queuing_time());

      Commit(*data);
      break;
    }
  }
  int64_t end_time = GetCurrentTime();
  global_stats_->AddExecutePrepareDelay(end_time-start_time);

  double ratio = static_cast<double>(execute_txns.size())/ txns.size();
  global_stats_->AddCommitRatio(ratio*100000);
  //LOG(ERROR)<<"exe time:"<<execute_txns.size()<<" total:"<<txns.size()<<" ratio:"<<ratio;

  return ;
}


}  // namespace tusk
}  // namespace resdb
