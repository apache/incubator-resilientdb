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

#include <google/protobuf/message.h>

#include "platform/statistic/stats.h"

namespace resdb {

enum TransactionStatue {
  None = 0,
  Prepare = -999,
  READY_PREPARE = 1,
  READY_COMMIT = 2,
  READY_EXECUTE = 3,
  EXECUTED = 4,
};

class TransactionCollector {
 public:
  TransactionCollector(int64_t seq)
      : seq_(seq), status_(TransactionStatue::None) {}

  ~TransactionCollector() = default;

  int AddRequest(
      std::unique_ptr<google::protobuf::Message> request, int sender_id,
      int type,
      std::function<void(const google::protobuf::Message&, int received_count,
                         std::atomic<TransactionStatue>* status)>
          call_back);

  TransactionStatue GetStatus() const;
  int64_t GetSeq() { return seq_; }

  std::unique_ptr<google::protobuf::Message> GetData() {
    return std::move(request_);
  }

 private:
  int64_t seq_;
  std::unique_ptr<google::protobuf::Message> request_;
  std::atomic<TransactionStatue> status_ = TransactionStatue::None;
  std::set<int> senders_[10];
};

}  // namespace resdb
