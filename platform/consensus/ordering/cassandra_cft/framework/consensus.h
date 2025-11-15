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

#include "executor/common/transaction_manager.h"
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/cassandra_cft/algorithm/cassandra.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace cassandra {

class Consensus : public common::Consensus {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  virtual ~Consensus() = default;

 private:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const Transaction& txn);

  int Prepare(const Transaction& txn);

 protected:
  std::unique_ptr<cassandra_recv::Cassandra> cassandra_;
  Stats* global_stats_;
  int64_t start_;
  std::mutex mutex_;
  int send_num_[200];
};

}  // namespace cassandra
}  // namespace resdb
