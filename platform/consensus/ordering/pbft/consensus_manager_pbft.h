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
