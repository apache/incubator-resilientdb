#include "ordering/pbft/transaction_collector.h"

#include <glog/logging.h>

#include "crypto/signature_verifier.h"

namespace resdb {

uint64_t TransactionCollector::Seq() { return seq_; }

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

int TransactionCollector::AddRequest(
    std::unique_ptr<Request> request, const SignatureInfo& signature,
    bool is_main_request,
    std::function<void(const Request&, int received_count, CollectorDataType*,
                       std::atomic<TransactionStatue>* status)>
        call_back) {
  if (request == nullptr) {
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
    LOG(ERROR) << "data invalid, seq not the same:" << seq
               << " collect seq:" << seq_;
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
    if (need_data_collection_) {
      auto request_info = std::make_unique<RequestInfo>();
      request_info->signature = signature;
      Request* raw_request = request.get();
      request_info->request = std::move(request);

      std::unique_lock<std::mutex> lk(mutex_);
      if (senders_[type].test(sender_id)) {
        LOG(ERROR) << "type:" << type << " sender id existed:";
        return 0;
      }
      senders_[type][sender_id] = 1;
      data_[type][hash].push_back(std::move(request_info));
      auto& it = data_[type][hash];
      call_back(*raw_request, senders_[type].count(), &it, &status_);
    } else {
      senders_[type][sender_id] = 1;
      call_back(*request, senders_[type].count(), nullptr, &status_);
    }
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
    executor_->Commit(std::move(main_request->request));
  }
  return 0;
}

}  // namespace resdb
