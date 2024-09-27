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
                                     SignatureVerifier* verifier)
    : config_(config),
      replica_communicator_(replica_communicator),
      txn_db_(std::make_unique<ChainState>()),
      verifier_(verifier),
      stop_(false),
      txn_accessor_(config),
      highest_prepared_seq_(0) {
  current_stable_seq_ = 0;
  if (config_.GetConfigData().enable_viewchange()) {
    config_.EnableCheckPoint(true);
  }
  if (config_.IsCheckPointEnabled()) {
    stable_checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateStableCheckPointStatus, this);
    checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateCheckPointStatus, this);
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
}

std::string GetHash(const std::string& h1, const std::string& h2) {
  return SignatureVerifier::CalculateHash(h1 + h2);
}

ChainState* CheckPointManager::GetTxnDB() { return txn_db_.get(); }

uint64_t CheckPointManager::GetMaxTxnSeq() { return txn_db_->GetMaxSeq(); }

uint64_t CheckPointManager::GetStableCheckpoint() {
  std::lock_guard<std::mutex> lk(mutex_);
  return current_stable_seq_;
}

StableCheckPoint CheckPointManager::GetStableCheckpointWithVotes() {
  std::lock_guard<std::mutex> lk(mutex_);
  return stable_ckpt_;
}

void CheckPointManager::AddCommitData(std::unique_ptr<Request> request) {
  if (config_.IsCheckPointEnabled()) {
    data_queue_.Push(std::move(request));
  } else {
    txn_db_->Put(std::move(request));
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
        if (it.second.size() >=
            static_cast<size_t>(config_.GetMinCheckpointReceiveNum())) {
          committable_seq_ = it.first.first;
          committable_hash_ = it.first.second;
          std::set<uint32_t> senders_ =
              sender_ckpt_[std::make_pair(committable_seq_, committable_hash_)];
          sem_post(&committable_seq_signal_);
          if (last_seq_ < committable_seq_ &&
              last_committable_seq < committable_seq_) {
            auto replicas_ = config_.GetReplicaInfos();
            for (auto& replica_ : replicas_) {
              std::string last_hash;
              uint64_t last_seq;
              {
                std::lock_guard<std::mutex> lk(lt_mutex_);
                last_hash = last_hash_;
                // last_seq_ = last_seq > last_committable_seq ? last_seq :
                // last_committable_seq;
                last_seq = last_seq_;
              }
              if (senders_.count(replica_.id()) &&
                  last_seq < committable_seq_) {
                // LOG(ERROR) << "GetRequestFromReplica " << last_seq_ + 1 << "
                // " << committable_seq_;
                auto requests = txn_accessor_.GetRequestFromReplica(
                    last_seq + 1, committable_seq_, replica_);
                if (requests.ok()) {
                  bool fail = false;
                  for (auto& request : *requests) {
                    if (SignatureVerifier::CalculateHash(request.data()) !=
                        request.hash()) {
                      LOG(ERROR)
                          << "The hash of the request does not match the data.";
                      fail = true;
                      break;
                    }
                    last_hash = GetHash(last_hash, request.hash());
                  }
                  if (fail) {
                    continue;
                  } else if (last_hash != committable_hash_) {
                    LOG(ERROR) << "The hash of requests returned do not match. "
                               << last_seq + 1 << " " << committable_seq_;
                  } else {
                    last_committable_seq = committable_seq_;
                    for (auto& request : *requests) {
                      if (executor_) {
                        executor_->Commit(std::make_unique<Request>(request));
                      }
                    }
                    SetHighestPreparedSeq(committable_seq_);
                    // LOG(ERROR) << "[4]";
                    break;
                  }
                }
              }
            }
          }
        }
        if (it.second.size() >=
            static_cast<size_t>(config_.GetMinDataReceiveNum())) {
          stable_seq = it.first.first;
          stable_hash = it.first.second;
        }
      }
      new_data_ = 0;
    }

    // LOG(ERROR) << "current stable seq:" << current_stable_seq_
    //  << " stable seq:" << stable_seq;
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
      // LOG(INFO) << "done. stable seq:" << current_stable_seq_
      //           << " votes:" << stable_ckpt_.DebugString();
      // LOG(INFO) << "done. stable seq:" << current_stable_seq_;
    }
    UpdateStableCheckPointCallback(current_stable_seq_);
  }
}

void CheckPointManager::SetTimeoutHandler(
    std::function<void()> timeout_handler) {
  timeout_handler_ = timeout_handler;
}

void CheckPointManager::TimeoutHandler() {
  if (timeout_handler_) {
    timeout_handler_();
  }
}

void CheckPointManager::UpdateCheckPointStatus() {
  uint64_t last_ckpt_seq = 0;
  int water_mark = config_.GetCheckPointWaterMark();
  int timeout_ms = config_.GetViewchangeCommitTimeout();
  std::vector<std::string> stable_hashs;
  std::vector<uint64_t> stable_seqs;
  while (!stop_) {
    auto request = data_queue_.Pop(timeout_ms);
    if (request == nullptr) {
      // if (last_seq > 0) {
      //   TimeoutHandler();
      // }
      continue;
    }
    std::string hash_ = request->hash();
    uint64_t current_seq = request->seq();
    if (current_seq != last_seq_ + 1) {
      LOG(ERROR) << "seq invalid:" << last_seq_ << " current:" << current_seq;
      continue;
    }
    {
      std::lock_guard<std::mutex> lk(lt_mutex_);
      last_hash_ = GetHash(last_hash_, request->hash());
      last_seq_++;
    }
    bool is_recovery = request->is_recovery();
    txn_db_->Put(std::move(request));

    if (current_seq == last_ckpt_seq + water_mark) {
      last_ckpt_seq = current_seq;
      if (!is_recovery) {
        BroadcastCheckPoint(last_ckpt_seq, last_hash_, stable_hashs,
                            stable_seqs);
      }
    }
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
  return highest_prepared_seq_;
}

void CheckPointManager::SetHighestPreparedSeq(uint64_t seq) {
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

}  // namespace resdb
