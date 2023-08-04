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

#pragma once

#include <bitset>

#include "platform/consensus/ordering/common/data_collector.h"
#include "platform/consensus/ordering/poe/proto/poe.pb.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

class QCCollector : public DataCollectorBasic {
 public:
  QCCollector(uint64_t seq) : seq_(seq), status_(TransactionStatue::None) {}

  ~QCCollector() = default;

  // Add a message and count by its hash value.
  // After it is done call_back will be triggered.
  int AddRequest(std::unique_ptr<Request> request, bool is_main_request,
                 std::function<void(const Request&, int received_count,
                                    std::atomic<TransactionStatue>* status)>
                     call_back);
  uint64_t Seq() { return seq_; }

  QC GetQC(uint64_t seq);

 private:
  uint64_t seq_;
  std::atomic<bool> is_committed_ = false;
  AtomicUniquePtr<RequestInfo> atomic_mian_request_;
  std::atomic<TransactionStatue> status_ = TransactionStatue::None;
  std::bitset<256> senders_[Request::NUM_OF_TYPE];
  std::mutex mutex_;
  QC commit_certs_;
};

}  // namespace resdb
