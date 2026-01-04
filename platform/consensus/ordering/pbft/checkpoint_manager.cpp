/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/consensus/ordering/pbft/checkpoint_manager.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/pbft/transaction_utils.h"
#include "platform/proto/checkpoint_info.pb.h"

namespace resdb {

CheckPointManager::CheckPointManager(const ResDBConfig& config,
                                     ReplicaCommunicator* replica_communicator,
                                     SignatureVerifier* verifier,
                                     SystemInfo * sys_info)
    : config_(config),
      replica_communicator_(replica_communicator),
      verifier_(verifier),
      stop_(false),
      txn_accessor_(config),
      highest_prepared_seq_(0),
      sys_info_(sys_info){
  current_stable_seq_ = 0;
  if (config_.GetConfigData().enable_viewchange()) {
    config_.EnableCheckPoint(true);
  }
  if (config_.IsCheckPointEnabled()) {
    stable_checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateStableCheckPointStatus, this);
    checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateCheckPointStatus, this);
    status_thread_ = std::thread(&CheckPointManager::SyncStatus, this);
  }
  sem_init(&committable_seq_signal_, 0, 0);
}

CheckPointManager::~CheckPointManager() { Stop(); }

void CheckPointManager::Stop() {
  stop_ = true;
  if (checkpoint_thread_.joinable()) {
    checkpoint_thread_.join();
  }
  if (stable_checkpoint_thread_.joinable()) {
    stable_checkpoint_thread_.join();
  }
  if (status_thread_.joinable()) {
    status_thread_.join();
  }
}

void CheckPointManager::SetResetExecute(std::function<void (uint64_t seq)> func) {
  reset_execute_func_ = func;
}

std::string GetHash(const std::string& h1, const std::string& h2) {
  return SignatureVerifier::CalculateHash(h1 + h2);
}

uint64_t CheckPointManager::GetStableCheckpoint() {
  std::lock_guard<std::mutex> lk(mutex_);
  return current_stable_seq_;
}

StableCheckPoint CheckPointManager::GetStableCheckpointWithVotes() {
  std::lock_guard<std::mutex> lk(mutex_);
  LOG(ERROR)<<"get stable ckpt:"<<stable_ckpt_.DebugString();
  return stable_ckpt_;
}

void CheckPointManager::AddCommitData(std::unique_ptr<Request> request) {
  LOG(ERROR)<<" add commit data:"<<request->seq();
  if (config_.IsCheckPointEnabled()) {
    data_queue_.Push(std::move(request));
  }
}

// check whether there are 2f+1 valid checkpoint proof.
bool CheckPointManager::IsValidCheckpointProof(
    const StableCheckPoint& stable_ckpt) {
  std::string hash = stable_ckpt_.hash();
  std::set<uint32_t> senders;
  for (const auto& signature : stable_ckpt_.signatures()) {
    if (!verifier_->VerifyMessage(hash, signature)) {
      return false;
    }
    senders.insert(signature.node_id());
  }

  return (static_cast<int>(senders.size()) >= config_.GetMinDataReceiveNum()) ||
         (stable_ckpt.seq() == 0 && senders.size() == 0);
}

int CheckPointManager::ProcessCheckPoint(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  CheckPointData checkpoint_data;
  if (!checkpoint_data.ParseFromString(request->data())) {
    LOG(ERROR) << "parse checkpont data fail:";
    return -2;
  }
  uint64_t checkpoint_seq = checkpoint_data.seq();
  uint32_t sender_id = request->sender_id();
  LOG(ERROR)<<" receive ckpt:"<<checkpoint_seq<<" from:"<<sender_id;
  int water_mark = config_.GetCheckPointWaterMark();
  if (checkpoint_seq % water_mark) {
    LOG(ERROR) << "checkpoint seq not invalid:" << checkpoint_seq;
    return -2;
  }

  if (verifier_) {
    // check signatures
    bool valid = verifier_->VerifyMessage(checkpoint_data.hash(),
                                          checkpoint_data.hash_signature());
    if (!valid) {
      LOG(ERROR) << "request is not valid:"
                 << checkpoint_data.hash_signature().DebugString();
      return -2;
    }
  }

  {
    std::lock_guard<std::mutex> lk(mutex_);
    auto res =
        sender_ckpt_[std::make_pair(checkpoint_seq, checkpoint_data.hash())]
            .insert(sender_id);
    if (res.second) {
      sign_ckpt_[std::make_pair(checkpoint_seq, checkpoint_data.hash())]
          .push_back(checkpoint_data.hash_signature());
      new_data_++;
    }
    if (sender_ckpt_[std::make_pair(checkpoint_seq, checkpoint_data.hash())]
            .size() == 1) {
      for (auto& hash_ : checkpoint_data.hashs()) {
        hash_ckpt_[std::make_pair(checkpoint_seq, checkpoint_data.hash())]
            .push_back(hash_);
      }
    }
    Notify();
  }
  return 0;
}

void CheckPointManager::Notify() {
  std::lock_guard<std::mutex> lk(cv_mutex_);
  cv_.notify_all();
}

bool CheckPointManager::Wait() {
  int timeout_ms = 1000;
  std::unique_lock<std::mutex> lk(cv_mutex_);
  return cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                      [&] { return new_data_ > 0; });
}

void CheckPointManager::CheckHealthy() {
  uint32_t current_time = time(nullptr);

  std::map<uint64_t, int> seqs;

  for(int i = 1; i <= config_.GetReplicaNum(); ++i) {
    if (last_update_time_.find(i) == last_update_time_.end() || last_update_time_[i] == 0) {
      continue;
    }
    LOG(ERROR)<<" check healthy, replica:"<<i
    <<" current time:"<<current_time
    <<" last time:"<<last_update_time_[i]
    <<" timeout:"<<replica_timeout_
    <<" pass:"<<current_time - last_update_time_[i];
    if( current_time - last_update_time_[i] > replica_timeout_ ) {
      TimeoutHandler(i);
    }
    seqs[status_[i]]++;
  }

  uint64_t unstable_check_ckpt = 0;
  for(auto it : seqs) {
    int num = 0;
    for(auto sit: seqs) {
      if(sit.first < it.first) {
        continue;
      }
      num += sit.second;
    }
    if(num >= config_.GetMinDataReceiveNum()) {
      unstable_check_ckpt = std::max(unstable_check_ckpt, it.first);
    }
  }
  SetUnstableCkpt(unstable_check_ckpt);
}

void CheckPointManager::UpdateStableCheckPointStatus() {
  uint64_t last_committable_seq = 0;
  while (!stop_) {
    if (!Wait()) {
      continue;
    }
    uint64_t stable_seq = 0;
    std::string stable_hash;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      for (auto it : sender_ckpt_) {
         LOG(ERROR)<<" get ckpt seq :"<<it.first.first<<" size:"<<it.second.size();
        if (it.second.size() >=
            static_cast<size_t>(config_.GetMinCheckpointReceiveNum())) {
          committable_seq_ = it.first.first;
          committable_hash_ = it.first.second;
          std::set<uint32_t> senders_ =
              sender_ckpt_[std::make_pair(committable_seq_, committable_hash_)];
          sem_post(&committable_seq_signal_);
        }
        if (it.second.size() >=
            static_cast<size_t>(config_.GetMinDataReceiveNum())) {
          stable_seq = it.first.first;
          stable_hash = it.first.second;
        }
      }
      new_data_ = 0;
    }

     LOG(ERROR) << "current stable seq:" << current_stable_seq_
      << " stable seq:" << stable_seq;
     if(stable_seq == 0) {
       continue;
     }
    std::vector<SignatureInfo> votes;
    if (current_stable_seq_ < stable_seq) {
      std::lock_guard<std::mutex> lk(mutex_);
      votes = sign_ckpt_[std::make_pair(stable_seq, stable_hash)];
      std::set<uint32_t> senders_ =
          sender_ckpt_[std::make_pair(stable_seq, stable_hash)];

      auto it = sender_ckpt_.begin();
      while (it != sender_ckpt_.end()) {
        if (it->first.first <= stable_seq) {
          sign_ckpt_.erase(sign_ckpt_.find(it->first));
          auto tmp = it++;
          sender_ckpt_.erase(tmp);
        } else {
          it++;
        }
      }
      stable_ckpt_.set_seq(stable_seq);
      stable_ckpt_.set_hash(stable_hash);
      stable_ckpt_.mutable_signatures()->Clear();
      for (auto vote : votes) {
        *stable_ckpt_.add_signatures() = vote;
      }
      current_stable_seq_ = stable_seq;
       LOG(INFO) << "done. stable seq:" << current_stable_seq_
                 << " votes:" << stable_ckpt_.DebugString();
       LOG(INFO) << "done. stable seq:" << current_stable_seq_;
    }
    UpdateStableCheckPointCallback(current_stable_seq_);
  }
}

void CheckPointManager::SetTimeoutHandler(
    std::function<void(int)> timeout_handler) {
  timeout_handler_ = timeout_handler;
}

void CheckPointManager::TimeoutHandler() {
  if (timeout_handler_) {
    timeout_handler_(0);
<<<<<<< HEAD
=======
  }
}

void CheckPointManager::TimeoutHandler(uint32_t replica) {
  if (timeout_handler_) {
    timeout_handler_(replica);
>>>>>>> master
  }
}

void CheckPointManager::TimeoutHandler(uint32_t replica) {
  if (timeout_handler_) {
    timeout_handler_(replica);
  }
}

void CheckPointManager::SetLastCommit(uint64_t seq) { 
  LOG(ERROR)<<" set last commit:"<<seq;
  last_seq_ = seq; 
  std::lock_guard<std::mutex> lk(lt_mutex_);
  committed_status_.clear();
}

uint64_t CheckPointManager::GetLastCommit() {
  return last_seq_;
}

int CheckPointManager::ProcessStatusSync(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  CheckPointData checkpoint_data;
  if (!checkpoint_data.ParseFromString(request->data())) {
    LOG(ERROR) << "parse checkpont data fail:";
    return -2;
  }
  uint64_t seq = checkpoint_data.seq();
  uint32_t sender_id = request->sender_id();
  uint32_t primary_id = checkpoint_data.primary_id();
  uint32_t view = checkpoint_data.view();

  status_[sender_id] = seq;
  last_update_time_[sender_id] = time(nullptr);
<<<<<<< HEAD
  view_status_[sender_id] = std::make_pair(primary_id, view);
  LOG(ERROR) << " received from :" << sender_id << " commit status:" << seq<<" primary:"<<primary_id<<" view:"<<view;
=======
  LOG(ERROR) << " received from :" << sender_id << " commit status:" << seq;
>>>>>>> master
  return 0;
}

void CheckPointManager::CheckStatus(uint64_t last_seq) {
  std::vector<uint64_t> seqs;
  for (auto it : status_) {
    seqs.push_back(it.second);
  }

  sort(seqs.begin(), seqs.end());
  int f = config_.GetMaxMaliciousReplicaNum();

  if (seqs.size() <= f + 1) {
    return;
  }
  //uint64_t min_seq = seqs[f + 1];
  uint64_t min_seq = seqs.back();

  LOG(ERROR) << " check last seq:" << last_seq << " max seq:" << min_seq;
  if (last_seq < min_seq) {
    // need recovery from others
    reset_execute_func_(last_seq + 1);
    BroadcastRecovery(last_seq + 1, std::min(min_seq, last_seq + 500));
  }
}

void CheckPointManager::CheckSysStatus() {
  int f = config_.GetMaxMaliciousReplicaNum();

  std::map<std::pair<int, uint64_t>, int > views;
  int current_primary = 0;
  uint64_t current_view= 0;
  for(auto it : view_status_) {
    views[it.second]++;
    if(views[it.second] >= 2 * f+1){
      current_primary = it.second.first;
      current_view = it.second.second;
    }
  }

  if(current_primary > 0 && current_primary != sys_info_->GetPrimaryId() && current_view > sys_info_->GetCurrentView()) {
    sys_info_->SetCurrentView(current_view);
    sys_info_->SetPrimary(current_primary);
    LOG(ERROR)<<" change to primary:"<<current_primary<<" view:"<<current_view;
  }

}


void CheckPointManager::SyncStatus() {
  uint64_t last_check_seq = 0;
  uint64_t last_time = 0;
  while (!stop_) {
    uint64_t last_seq = last_seq_;

    CheckPointData checkpoint_data;
    std::unique_ptr<Request> checkpoint_request = NewRequest(
        Request::TYPE_STATUS_SYNC, Request(), config_.GetSelfInfo().id());
    checkpoint_data.set_seq(last_seq);
    checkpoint_data.set_view(sys_info_->GetCurrentView());
    checkpoint_data.set_primary_id(sys_info_->GetPrimaryId());
    checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
    replica_communicator_->BroadCast(*checkpoint_request);

    LOG(ERROR) << " sync status last seq:" << last_seq
               << " last time:" << last_time
               << " primary:" << sys_info_->GetPrimaryId()
               << " view:" << sys_info_->GetCurrentView();
    if (last_check_seq == last_seq && last_time > 5) {
      CheckStatus(last_seq);
      last_time = 0;
    }
    CheckSysStatus();

    if (last_seq != last_check_seq) {
      last_check_seq = last_seq;
      last_time = 0;
    }
    CheckHealthy();
    sleep(10);
    last_time++;
  }
}

void CheckPointManager::UpdateCheckPointStatus() {
  uint64_t last_ckpt_seq = 0;
  int water_mark = config_.GetCheckPointWaterMark();
  int timeout_ms = config_.GetViewchangeCommitTimeout();
  std::vector<std::string> stable_hashs;
  std::vector<uint64_t> stable_seqs;
  std::map<uint64_t, std::unique_ptr<Request>> pendings;
  while (!stop_) {
    std::unique_ptr<Request> request = nullptr;
    if (!pendings.empty()) {
      LOG(ERROR)<<" last seq:"<<last_seq_<<" pending:"<<pendings.begin()->second->seq();
      if (pendings.begin()->second->seq() == last_seq_ + 1) {
        request = std::move(pendings.begin()->second);
        pendings.erase(pendings.begin());
      }
    }
    if (request == nullptr) {
      request = data_queue_.Pop(timeout_ms);
    }
    if (request == nullptr) {
      continue;
    }
    std::string hash_ = request->hash();
    uint64_t current_seq = request->seq();
    LOG(ERROR) << "update checkpoint seq :" << last_seq_ << " current:" << current_seq;
    if (current_seq != last_seq_ + 1) {
      LOG(ERROR) << "seq invalid:" << last_seq_ << " current:" << current_seq;
      if (current_seq > last_seq_ + 1) {
        pendings[current_seq] = std::move(request);
      }
      continue;
    }
    {
      std::lock_guard<std::mutex> lk(lt_mutex_);
      last_hash_ = GetHash(last_hash_, request->hash());
      last_seq_++;
    }
    bool is_recovery = request->is_recovery();

    LOG(ERROR)<<" current seq:"<<current_seq<<" water mark:"<<water_mark<<" current stable seq:"<<current_stable_seq_;
    if (current_seq > 0 && current_seq % water_mark == 0) {
      last_ckpt_seq = current_seq;
      //if (!is_recovery) {
      BroadcastCheckPoint(last_ckpt_seq, last_hash_, stable_hashs,
                            stable_seqs);
      //}
    }
    ClearCommittedStatus(current_seq);
  }
  return;
}

void CheckPointManager::BroadcastCheckPoint(
    uint64_t seq, const std::string& hash,
    const std::vector<std::string>& stable_hashs,
    const std::vector<uint64_t>& stable_seqs) {
  CheckPointData checkpoint_data;
  std::unique_ptr<Request> checkpoint_request = NewRequest(
      Request::TYPE_CHECKPOINT, Request(), config_.GetSelfInfo().id());
  checkpoint_data.set_seq(seq);
  checkpoint_data.set_hash(hash);
  if (verifier_) {
    auto signature_or = verifier_->SignMessage(hash);
    if (!signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return;
    }
    *checkpoint_data.mutable_hash_signature() = *signature_or;
  }

  checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
  replica_communicator_->BroadCast(*checkpoint_request);
}

void CheckPointManager::BroadcastRecovery(uint64_t min_seq, uint64_t max_seq) {
  RecoveryRequest recovery_data;
  std::unique_ptr<Request> recovery_request = NewRequest(
      Request::TYPE_RECOVERY_DATA, Request(), config_.GetSelfInfo().id());
  recovery_data.set_min_seq(min_seq);
  recovery_data.set_max_seq(max_seq);
  recovery_data.SerializeToString(recovery_request->mutable_data());

  LOG(ERROR) << " recovery request [" << min_seq << "," << max_seq << "]";
  replica_communicator_->BroadCast(*recovery_request);
}

void CheckPointManager::WaitSignal() {
  std::unique_lock<std::mutex> lk(mutex_);
  signal_.wait(lk, [&] { return !stable_hash_queue_.Empty(); });
}

std::unique_ptr<std::pair<uint64_t, std::string>>
CheckPointManager::PopStableSeqHash() {
  return stable_hash_queue_.Pop();
}

uint64_t CheckPointManager::GetHighestPreparedSeq() {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  LOG(ERROR)<<"get high prepared seq:"<<highest_prepared_seq_;
  return highest_prepared_seq_;
}

void CheckPointManager::SetHighestPreparedSeq(uint64_t seq) {
  LOG(ERROR)<<"set high prepared seq:"<<seq;
  std::lock_guard<std::mutex> lk(lt_mutex_);
  highest_prepared_seq_ = seq;
}

sem_t* CheckPointManager::CommitableSeqSignal() {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  return &committable_seq_signal_;
}

uint64_t CheckPointManager::GetCommittableSeq() {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  return committable_seq_;
}

void CheckPointManager::SetUnstableCkpt(uint64_t unstable_check_ckpt) {
  LOG(ERROR)<<" set unstable ckpt:"<<unstable_check_ckpt;
  {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  unstable_check_ckpt_ = unstable_check_ckpt;
  }
}

uint64_t CheckPointManager::GetUnstableCkpt() {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  LOG(ERROR)<<" get unstable ckpt:"<<unstable_check_ckpt_;
  return unstable_check_ckpt_;
}

void CheckPointManager::AddCommitState(uint64_t seq) {
  LOG(ERROR)<<" add commited state:"<<seq;
  std::lock_guard<std::mutex> lk(lt_mutex_);
  committed_status_[seq] = true;
}

bool CheckPointManager::IsCommitted(uint64_t seq) {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  if(seq < last_seq_) {
    return true;
  }
  return committed_status_.find(seq) != committed_status_.end();
}

void CheckPointManager::ClearCommittedStatus(uint64_t seq) {
  std::lock_guard<std::mutex> lk(lt_mutex_);
  while(!committed_status_.empty()){
    if(committed_status_.begin()->first <= seq) {
      committed_status_.erase(committed_status_.begin());
    }
    else {
      break;
    }
  }
}

// void CheckPointManager::SetLastExecutedSeq(uint64_t latest_executed_seq){
//   latest_executed_seq = executor_->get_latest_executed_seq();
// }

}  // namespace resdb
