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

#include "platform/consensus/ordering/poe/qc_collector.h"

#include <glog/logging.h>

namespace resdb {

int QCCollector::AddRequest(
    std::unique_ptr<Request> request, bool is_main_request,
    std::function<void(const Request&, int received_count,
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
    call_back(*main_request->request.get(), 1, &status_);
    return 0;
  } else {
    if (request->has_data_signature() &&
        request->data_signature().node_id() > 0) {
      std::lock_guard<std::mutex> lk(mutex_);
      *commit_certs_.add_signatures() = request->data_signature();
    } else {
      LOG(ERROR) << "no sig ===== " << seq_;
    }
    senders_[type][sender_id] = 1;
    call_back(*request, senders_[type].count(), &status_);
  }
  return 0;
}

QC QCCollector::GetQC(uint64_t seq) {
  if (seq != seq_) return QC();
  std::lock_guard<std::mutex> lk(mutex_);
  return commit_certs_;
}

}  // namespace resdb
