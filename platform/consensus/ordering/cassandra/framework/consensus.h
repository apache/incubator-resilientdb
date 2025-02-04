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
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/cassandra/algorithm/basic/cassandra.h"
#include "platform/consensus/ordering/cassandra/algorithm/fast_path/cassandra.h"
#include "platform/consensus/ordering/cassandra/algorithm/mem/cassandra.h"
#include "platform/consensus/ordering/cassandra/algorithm/mem_recovery/cassandra.h"
#include "platform/consensus/ordering/cassandra/framework/performance_manager.h"
#include "platform/consensus/ordering/cassandra/framework/response_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace cassandra {

class Consensus : public ConsensusManager {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  virtual ~Consensus() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;
  std::vector<ReplicaInfo> GetReplicas() override;

  void SetupPerformanceDataFunc(std::function<std::string()> func);

  void SetCommunicator(ReplicaCommunicator* replica_communicator);

 private:
  int SendMsg(int type, const google::protobuf::Message& msg, int node_id);
  int Broadcast(int type, const google::protobuf::Message& msg);
  int Commit(const Transaction& msg);
  int Prepare(const Transaction& msg);
  int ResponseMsg(const BatchUserResponse& batch_resp);

 protected:
  ReplicaCommunicator* replica_communicator_;
  std::unique_ptr<PerformanceManager> performance_manager_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  // std::unique_ptr<basic::Cassandra> cassandra_;
  // std::unique_ptr<cassandra_mem::Cassandra> cassandra_;
  // std::unique_ptr<cassandra_fp::Cassandra> cassandra_;
  std::unique_ptr<cassandra_recv::Cassandra> cassandra_;
  Stats* global_stats_;
  std::atomic<int64_t> receive_msg_;
  int64_t start_;
  std::mutex mutex_;
  int send_num_[200];
};

}  // namespace cassandra
}  // namespace resdb
