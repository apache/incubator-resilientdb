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

#include "platform/consensus/ordering/pbft/transaction_collector.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"

namespace resdb {

uint64_t TransactionCollector::Seq() { return seq_; }

bool TransactionCollector::IsPrepared() { return is_prepared_; }

TransactionStatue TransactionCollector::GetStatus() const { return status_; }

int TransactionCollector::SetContextList(
    uint64_t seq, std::vector<std::unique_ptr<Context>> context) {
  if (seq != seq_) {
    return -2;
  }
  context_list_ = std::move(context);
  return 0;
}

bool TransactionCollector::HasClientContextList(uint64_t seq) const {
  if (seq != seq_) {
    return false;
  }
  return !context_list_.empty();
}

std::vector<std::unique_ptr<Context>> TransactionCollector::FetchContextList(
    uint64_t seq) {
  if (seq != seq_) {
    return std::vector<std::unique_ptr<Context>>();
  }
  return std::move(context_list_);
}

std::vector<RequestInfo> TransactionCollector::GetPreparedProof() {
  std::vector<RequestInfo> prepared_info;
  for (const auto& proof : prepared_proof_) {
    RequestInfo info;
    info.signature = proof->signature;
    info.request = std::make_unique<Request>(*proof->request);
    prepared_info.push_back(std::move(info));
  }
  return prepared_info;
}

int TransactionCollector::AddRequest(
    std::unique_ptr<Request> request, const SignatureInfo& signature,
    bool is_main_request,
    std::function<void(const Request&, int received_count, CollectorDataType*,
                       std::atomic<TransactionStatue>* status, bool force)>
        call_back) {
  if (request == nullptr) {
    LOG(ERROR) << "request empty";
    return -2;
  }

  int32_t sender_id = request->sender_id();
  std::string hash = request->hash();
  int type = request->type();
  uint64_t seq = request->seq();
  uint64_t view = request->current_view();
  if (is_committed_) {
    return -2;
  }
  if (status_.load() == EXECUTED) {
    return -2;
  }

  if (seq_ != static_cast<uint64_t>(request->seq())) {
    // LOG(ERROR) << "data invalid, seq not the same:" << seq
    //           << " collect seq:" << seq_;
    return -2;
  }

  if (is_main_request) {
    auto request_info = std::make_unique<RequestInfo>();
    request_info->signature = signature;
    request_info->request = std::move(request);
    bool force = false;
    if (view_ && view_ < view && !is_prepared_) {
      force = true;
      atomic_mian_request_.Clear();
    }
    int ret = atomic_mian_request_.Set(request_info);
    if (!ret) {
      other_main_request_.insert(std::move(request_info));
      LOG(ERROR) << "set main request fail: data existed:" << seq
                 << " ret:" << ret;
      return -2;
    }
    auto main_request = atomic_mian_request_.Reference();
    if (main_request->request == nullptr) {
      LOG(ERROR) << "set main request data fail";
      return -2;
    }
    view_ = view;
    call_back(*main_request->request.get(), 1, nullptr, &status_, force);
    return 0;
  } else {
    if (enable_viewchange_) {
      if (type == Request::TYPE_PREPARE) {
        if (status_.load() <= TransactionStatue::READY_PREPARE) {
          auto request_info = std::make_unique<RequestInfo>();
          request_info->signature = signature;
          request_info->request = std::make_unique<Request>(*request);
          std::lock_guard<std::mutex> lk(mutex_);
          if (is_prepared_) {
            return 0;
          }
          prepared_proof_.push_back(std::move(request_info));
          if (senders_[type].count(hash) == 0) {
            senders_[type].insert(std::make_pair(hash, std::bitset<128>()));
          }
          senders_[type][hash][sender_id] = 1;
          call_back(*request, senders_[type][hash].count(), nullptr, &status_,
                    false);
          if (status_.load() == TransactionStatue::READY_COMMIT) {
            is_prepared_ = true;
            if (atomic_mian_request_.Reference() != nullptr &&
                atomic_mian_request_.Reference()->request->hash() != hash) {
              atomic_mian_request_.Clear();
              for (auto it = other_main_request_.begin();
                   it != other_main_request_.end(); it++) {
                if ((*it)->request->hash() == hash) {
                  auto request_info = std::make_unique<RequestInfo>();
                  request_info->signature = (*it)->signature;
                  request_info->request = std::move((*it)->request);
                  atomic_mian_request_.Set(request_info);
                  break;
                }
              }
              other_main_request_.clear();
            }
            int pos = 0;
            for (size_t i = 0; i < prepared_proof_.size(); i++) {
              if (prepared_proof_[i]->request->hash() == hash) {
                prepared_proof_[pos++] = std::move(prepared_proof_[i]);
              }
            }
            prepared_proof_.erase(prepared_proof_.begin() + pos,
                                  prepared_proof_.end());
          }
        }
        return 0;
      }
    }
    if (request->type() == Request::TYPE_COMMIT) {
      if (request->has_data_signature() &&
          request->data_signature().node_id() > 0) {
        std::lock_guard<std::mutex> lk(mutex_);
        LOG(ERROR) << "add qc signature";
        commit_certs_.push_back(request->data_signature());
      }
    }

    {
      std::lock_guard<std::mutex> lk(mutex_);
      if (senders_[type].count(hash) == 0) {
        senders_[type].insert(std::make_pair(hash, std::bitset<128>()));
      }
      senders_[type][hash][sender_id] = 1;
      call_back(*request, senders_[type][hash].count(), nullptr, &status_,
                false);
    }

    if (status_.load() == TransactionStatue::READY_EXECUTE) {
      Commit();
      return 1;
    }
  }
  return 0;
}

int TransactionCollector::Commit() {
  TransactionStatue old_status = TransactionStatue::READY_EXECUTE;
  bool res = status_.compare_exchange_strong(
      old_status, TransactionStatue::EXECUTED, std::memory_order_acq_rel,
      std::memory_order_acq_rel);
  if (!res) {
    return -2;
  }

  auto main_request = atomic_mian_request_.Reference();
  if (main_request == nullptr) {
    LOG(ERROR) << "no main";
    return -2;
  }

  is_committed_ = true;
  if (executor_ && main_request->request) {
    if (!commit_certs_.empty()) {
      for (const auto& sig : commit_certs_) {
        *main_request->request->mutable_committed_certs()
             ->add_committed_certs() = sig;
        // LOG(ERROR) << "add sig:" << sig.DebugString();
      }
    }
    executor_->Commit(std::move(main_request->request));
  }
  return 0;
}

std::vector<std::string> TransactionCollector::GetAllStoredHash() {
  std::vector<std::string> v;
  auto main_request = atomic_mian_request_.Reference();
  if (main_request) {
    v.push_back(main_request->request->hash());
  }
  for (auto& info : other_main_request_) {
    v.push_back(info->request->hash());
  }
  return v;
}

}  // namespace resdb
