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
    [&](std::vector<std::unique_ptr<Transaction>> txns){
      return CommitTxns(std::move(txns));
    });
  local_time_ = 0;
  execute_id_ = 1;
  global_stats_ = Stats::GetGlobalStats();
}

FairDAG::~FairDAG() {
}

bool FairDAG::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
    std::unique_lock<std::mutex> lk(mutex_);
  txn->set_timestamp(local_time_++);
  txn->set_proposer(id_);
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

uint64_t FairDAG::TryAssign(const std::string& hash) {
    const auto& commit_timelist = committed_txns_[hash];
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

uint64_t FairDAG::GetLowerboundTimestamp(const std::string& hash) {
    const auto& commit_timelist = committed_txns_[hash];
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
    //LOG(ERROR)<<"get uncommit time lowbound:"<<times[f_];
    return times[f_];
}

uint64_t FairDAG::GetCommitTimestamp(const std::string& hash) {
  const auto& commit_timelist = committed_txns_[hash];
  std::vector<uint64_t> times;
  for(int i = 1; i <= replica_num_; ++i){
    auto it = commit_timelist.find(i);
    if(it != commit_timelist.end()){
      times.push_back(it->second);
    //LOG(ERROR)<<"commit time:"<<it->second<<" from:"<<it->first;
    }
  }
  assert(static_cast<int>(times.size())>=2*f_+1);
    sort(times.begin(), times.end());
  //LOG(ERROR)<<"assign commit time:"<<times[f_+1];
  return times[f_];
}

void FairDAG::CommitTxns(std::vector<std::unique_ptr<Transaction> > txns) {
    int64_t start_time = GetCurrentTime();
  std::vector<std::unique_ptr<Transaction>> execute_txns;
  for(auto& txn : txns){
    committed_txns_[txn->hash()].insert(std::make_pair(txn->proposer(), txn->timestamp()));
    min_timestamp_[txn->proposer()] = std::max(min_timestamp_[txn->proposer()], txn->timestamp()+1);
    //LOG(ERROR)<<"proposer:"<<txn->proposer()<<" next commit time:"<<min_timestamp_[txn->proposer()]<<" timestamp:"<<txn->timestamp();
  }


  uint64_t lower_bound_uncommtted_time = -1;
  std::map<std::string, uint64_t> assign_time;
  for(auto& txn : txns){
    //LOG(ERROR)<<"commit from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq()<<" time num:"<<committed_txns_[txn->hash()].size()<<" proposer:"<<txn->proposer()<<" timestamp:"<<txn->timestamp()<<" lowerbound:"<<lower_bound_uncommtted_time;
    if(static_cast<int>(committed_txns_[txn->hash()].size()) < 2*f_+1){
      uint64_t try_time = TryAssign(txn->hash());
      if(try_time == 0){
        uint64_t lower_bound_time = GetLowerboundTimestamp(txn->hash());
        lower_bound_uncommtted_time = std::min(lower_bound_uncommtted_time, lower_bound_time);
      }
      else {
        assign_time[txn->hash()] = try_time;
        //LOG(ERROR)<<"spculative assign:"<<try_time;
      }
    }
    else {
      assign_time[txn->hash()] = GetCommitTimestamp(txn->hash());
    }
  }

  //LOG(ERROR)<<" lower_bound uncommitted time:"<<lower_bound_uncommtted_time;
  for(auto it = not_ready_.begin(); it != not_ready_.end();){
    if(it->first <lower_bound_uncommtted_time){
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

  for(auto& txn : txns){
    if(assign_time.find(txn->hash()) != assign_time.end()){
      //LOG(ERROR)<<"commit from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq()<<" time num:"<<committed_txns_[txn->hash()].size()<<" assign time:"<<assign_time[txn->hash()]<<" lower bound:"<<lower_bound_uncommtted_time;
      assert(assign_time.find(txn->hash()) != assign_time.end());
      /*
      if(static_cast<int>(committed_txns_[txn->hash()].size()) == replica_num_){
        assert(assign_time[txn->hash()] < lower_bound_uncommtted_time);
      }
      */
      if(assign_time[txn->hash()] < lower_bound_uncommtted_time){
        if(ready_.find(txn->hash()) == ready_.end()){
          ready_.insert(txn->hash());
          execute_txns.push_back(std::move(txn));
        }
      }
      else {
        //LOG(ERROR)<<"set to pending from:"<<txn->proxy_id()<<" user seq:"<<txn->user_seq()<<" proposer:"<<txn->proposer();
        not_ready_.insert(std::make_pair(assign_time[txn->hash()], std::move(txn)));
      }
    }
  }


  SortTxn(execute_txns, assign_time);

  //LOG(ERROR)<<"exe time:"<<execute_txns.size();
  for(auto& txn: execute_txns){
    txn->set_id(execute_id_++);
    //LOG(ERROR)<<"commit from proposer:"<<txn->proxy_id()<<" time:"<<txn->timestamp()<<" seq:"<<txn->user_seq();
    Commit(*txn);
  }
  int64_t end_time = GetCurrentTime();
  global_stats_->AddExecutePrepareDelay(end_time-start_time);
}


}  // namespace tusk
}  // namespace resdb
