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

#include "executor/common/custom_query.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/commitment.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/consensus/ordering/pbft/performance_manager.h"
#include "platform/consensus/ordering/pbft/query.h"
#include "platform/consensus/ordering/pbft/response_manager.h"
#include "platform/consensus/ordering/pbft/viewchange_manager.h"
#include "platform/consensus/recovery/recovery.h"
#include "platform/networkstrate/consensus_manager.h"

namespace resdb {

class ConsensusManagerPBFT : public ConsensusManager {
 public:
  ConsensusManagerPBFT(const ResDBConfig& config,
                       std::unique_ptr<TransactionManager> executor,
                       std::unique_ptr<CustomQuery> query_executor = nullptr);
  virtual ~ConsensusManagerPBFT() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

  std::vector<ReplicaInfo> GetReplicas() override;
  uint32_t GetPrimary() override;
  uint32_t GetVersion() override;

  void SetPrimary(uint32_t primary, uint64_t version) override;

  void Start() override;
  void SetupPerformanceDataFunc(std::function<std::string()> func);

  void SetPreVerifyFunc(std::function<bool(const Request&)>);
  void SetNeedCommitQC(bool need_qc);

 protected:
  int InternalConsensusCommit(std::unique_ptr<Context> context,
                              std::unique_ptr<Request> request);
  void AddPendingRequest(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  void AddComplainedRequest(std::unique_ptr<Context> context,
                            std::unique_ptr<Request> request);
  absl::StatusOr<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
  PopPendingRequest();
  absl::StatusOr<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
  PopComplainedRequest();

 protected:
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<CheckPointManager> checkpoint_manager_;
  std::unique_ptr<MessageManager> message_manager_;
  std::unique_ptr<Commitment> commitment_;
  std::unique_ptr<Query> query_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<PerformanceManager> performance_manager_;
  std::unique_ptr<ViewChangeManager> view_change_manager_;
  std::unique_ptr<Recovery> recovery_;
  Stats* global_stats_;
  std::queue<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
      request_pending_;
  std::queue<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
      request_complained_;
  std::mutex mutex_;
};

}  // namespace resdb
