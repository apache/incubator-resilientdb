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

#include "config/resdb_config.h"
#include "ordering/pbft/checkpoint.h"
#include "ordering/pbft/commitment.h"
#include "ordering/pbft/performance_manager.h"
#include "ordering/pbft/query.h"
#include "ordering/pbft/recovery.h"
#include "ordering/pbft/response_manager.h"
#include "ordering/pbft/transaction_manager.h"
#include "server/consensus_service.h"

namespace resdb {

class ConsensusServicePBFT : public ConsensusService {
 public:
  ConsensusServicePBFT(const ResDBConfig& config,
                       std::unique_ptr<TransactionExecutorImpl> executor);
  virtual ~ConsensusServicePBFT() = default;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

  std::vector<ReplicaInfo> GetReplicas() override;

  void Start();

 protected:
  virtual void AddNewReplica(const ReplicaInfo& info);

 protected:
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<CheckPoint> checkpoint_;
  std::unique_ptr<TransactionManager> transaction_manager_;
  std::unique_ptr<Commitment> commitment_;
  std::unique_ptr<Recovery> recovery_;
  std::unique_ptr<Query> query_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<PerformanceManager> performance_manager_;
  Stats* global_stats_;
};

}  // namespace resdb
