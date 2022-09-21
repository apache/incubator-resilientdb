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
