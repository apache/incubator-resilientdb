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

#include "ordering/pbft/transaction_collector.h"

#include <glog/logging.h>

#include "crypto/signature_verifier.h"

namespace resdb {

uint64_t TransactionCollector::Seq() { return seq_; }

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
                       std::atomic<TransactionStatue>* status)>
        call_back) {
  if (request == nullptr) {
    LOG(ERROR) << "request empty";
    return -2;
  }

  int32_t sender_id = request->sender_id();
  std::string hash = request->hash();
  int type = request->type();
  uint64_t seq = request->seq();
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
    int ret = atomic_mian_request_.Set(request_info);
    if (!ret) {
      LOG(ERROR) << "set main request fail: data existed:" << seq
                 << " ret:" << ret;
      return -2;
    }
    auto main_request = atomic_mian_request_.Reference();
    if (main_request->request == nullptr) {
      LOG(ERROR) << "set main request data fail";
      return -2;
    }
    call_back(*main_request->request.get(), 1, nullptr, &status_);
    return 0;
  } else {
    if (enable_viewchange_) {
      if (status_.load() == READY_PREPARE) {
        auto request_info = std::make_unique<RequestInfo>();
        request_info->signature = signature;
        request_info->request = std::make_unique<Request>(*request);
        prepared_proof_.push_back(std::move(request_info));
      }
    }
    if (request->has_data_signature() &&
        request->data_signature().node_id() > 0) {
      std::lock_guard<std::mutex> lk(mutex_);
      commit_certs_.push_back(request->data_signature());
    }
    senders_[type][sender_id] = 1;
    call_back(*request, senders_[type].count(), nullptr, &status_);
    if (status_.load() == TransactionStatue::READY_EXECUTE) {
      Commit();
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

}  // namespace resdb
