/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "ordering/pbft/checkpoint_manager.h"

#include "glog/logging.h"
#include "ordering/pbft/transaction_utils.h"
#include "proto/checkpoint_info.pb.h"

namespace resdb {

CheckPointManager::CheckPointManager(const ResDBConfig& config,
                                     ResDBReplicaClient* replica_client,
                                     SignatureVerifier* verifier)
    : config_(config),
      replica_client_(replica_client),
      txn_db_(std::make_unique<TxnMemoryDB>()),
      verifier_(verifier),
      stop_(false) {
  current_stable_seq_ = 0;
  if (config_.IsCheckPointEnabled()) {
    stable_checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateStableCheckPointStatus, this);
    checkpoint_thread_ =
        std::thread(&CheckPointManager::UpdateCheckPointStatus, this);
  }
}

CheckPointManager::~CheckPointManager() {
  stop_ = true;
  if (checkpoint_thread_.joinable()) {
    checkpoint_thread_.join();
  }
  if (stable_checkpoint_thread_.joinable()) {
    stable_checkpoint_thread_.join();
  }
}

TxnMemoryDB* CheckPointManager::GetTxnDB() { return txn_db_.get(); }

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
  return (senders.size() >= config_.GetMinDataReceiveNum()) ||
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
            static_cast<size_t>(config_.GetMinDataReceiveNum())) {
          stable_seq = it.first.first;
          stable_hash = it.first.second;
        }
      }
      new_data_ = 0;
    }

    LOG(ERROR) << "current stable seq:" << current_stable_seq_
               << " stable seq:" << stable_seq;
    std::vector<SignatureInfo> votes;
    if (current_stable_seq_ < stable_seq) {
      std::lock_guard<std::mutex> lk(mutex_);
      votes = sign_ckpt_[std::make_pair(stable_seq, stable_hash)];

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
      for (auto vote : votes) {
        *stable_ckpt_.add_signatures() = vote;
      }
      current_stable_seq_ = stable_seq;
      LOG(INFO) << "done. stable seq:" << current_stable_seq_
                << " votes:" << stable_ckpt_.DebugString();
    }
    UpdateStableCheckPointCallback(current_stable_seq_);
  }
}

std::string GetHash(const std::string& h1, const std::string& h2) {
  return SignatureVerifier::CalculateHash(h1 + h2);
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
  uint64_t last_seq = 0;
  uint64_t last_ckpt_seq = 0;
  std::string last_hash;
  int water_mark = config_.GetCheckPointWaterMark();
  int timeout_ms = config_.GetViewchangeCommitTimeout();
  while (!stop_) {
    auto request = data_queue_.Pop(timeout_ms);
    if (request == nullptr) {
      TimeoutHandler();
      continue;
    }
    uint64_t current_seq = request->seq();
    if (current_seq != last_seq + 1) {
      LOG(ERROR) << "seq invalid:" << last_seq << " current:" << current_seq;
      continue;
    }
    last_hash = GetHash(last_hash, request->hash());
    last_seq++;
    txn_db_->Put(std::move(request));

    if (current_seq == last_ckpt_seq + water_mark) {
      last_ckpt_seq = current_seq;
      BroadcastCheckPoint(last_ckpt_seq, last_hash);
    }
  }
  return;
}

void CheckPointManager::BroadcastCheckPoint(uint64_t seq,
                                            const std::string& hash) {
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
  replica_client_->BroadCast(*checkpoint_request);
}

}  // namespace resdb
