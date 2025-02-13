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

#pragma once

#include "executor/common/transaction_manager.h"
#include "platform/consensus/execution/transaction_executor.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/common/framework/performance_manager.h"
#include "platform/consensus/ordering/common/framework/response_manager.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace common {

class Consensus : public ConsensusManager {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  virtual ~Consensus();

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;
  std::vector<ReplicaInfo> GetReplicas() override;

  void SetupPerformanceDataFunc(std::function<std::string()> func);

  void SetCommunicator(ReplicaCommunicator* replica_communicator);

  void InitProtocol(ProtocolBase* protocol);

 protected:
  virtual int ProcessCustomConsensus(std::unique_ptr<Request> request);
  virtual int ProcessNewTransaction(std::unique_ptr<Request> request);
  virtual int CommitMsg(const google::protobuf::Message& msg);

 protected:
  int SendMsg(int type, const google::protobuf::Message& msg, int node_id);
  int Broadcast(int type, const google::protobuf::Message& msg);
  int ResponseMsg(const BatchUserResponse& batch_resp);
  void AsyncSend();
  bool IsStop();

 protected:
  void Init();
  void SetPerformanceManager(
      std::unique_ptr<PerformanceManager> performance_manger);

 protected:
  ReplicaCommunicator* replica_communicator_;
  std::unique_ptr<PerformanceManager> performance_manager_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<TransactionExecutor> transaction_executor_;
  Stats* global_stats_;

  LockFreeQueue<BatchUserResponse> resp_queue_;
  std::vector<std::thread> send_thread_;
  bool is_stop_;
};

}  // namespace common
}  // namespace resdb
