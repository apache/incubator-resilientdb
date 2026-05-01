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
#include "platform/consensus/ordering/common/framework/consensus.h"
#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"
#include "platform/consensus/ordering/raft/algorithm/raft.h"
#include "platform/consensus/ordering/raft/framework/raft_checkpoint_manager.h"
#include "platform/consensus/recovery/raft_recovery.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {
namespace raft {

class Consensus : public common::Consensus {
 public:
  Consensus(const ResDBConfig& config,
            std::unique_ptr<TransactionManager> transaction_manager);
  virtual ~Consensus() = default;

 private:
  int ProcessCustomConsensus(std::unique_ptr<Request> request) override;
  int ProcessNewTransaction(std::unique_ptr<Request> request) override;
  int CommitMsg(const google::protobuf::Message& msg) override;
  int CommitMsgInternal(const AppendEntries& txn);
  int ResponseMsg(const BatchUserResponse& batch_resp) override;
  void RecoverFromLogs();
  void OnCheckpointFinish(uint64_t seq);

 protected:
  std::unique_ptr<Raft> raft_;
  std::unique_ptr<LeaderElectionManager> leader_election_manager_;
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<RaftCheckPoint> raft_checkpoint_manager_;
  std::unique_ptr<RaftRecovery> recovery_;
  uint32_t snapshot_interval_ = 1000;
  uint64_t last_applied_ = 0;
  uint32_t last_snapshot_initiated_at_ = 0;
};

}  // namespace raft
}  // namespace resdb
