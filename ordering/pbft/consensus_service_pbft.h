#pragma once

#include "config/resdb_config.h"
#include "ordering/pbft/checkpoint_manager.h"
#include "ordering/pbft/commitment.h"
#include "ordering/pbft/performance_manager.h"
#include "ordering/pbft/query.h"
#include "ordering/pbft/response_manager.h"
#include "ordering/pbft/transaction_manager.h"
#include "ordering/pbft/viewchange_manager.h"
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
  uint32_t GetPrimary() override;
  uint32_t GetVersion() override;
  // only for client nodes.
  void SetPrimary(uint32_t primary, uint64_t version);

  void Start();
  void SetupPerformanceDataFunc(std::function<std::string()> func);

 protected:
  int InternalConsensusCommit(std::unique_ptr<Context> context,
                              std::unique_ptr<Request> request);
  void AddPendingRequest(std::unique_ptr<Context> context,
                         std::unique_ptr<Request> request);
  absl::StatusOr<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
  PopPendingRequest();

 protected:
  std::unique_ptr<SystemInfo> system_info_;
  std::unique_ptr<CheckPointManager> checkpoint_manager_;
  std::unique_ptr<TransactionManager> transaction_manager_;
  std::unique_ptr<Commitment> commitment_;
  std::unique_ptr<Query> query_;
  std::unique_ptr<ResponseManager> response_manager_;
  std::unique_ptr<PerformanceManager> performance_manager_;
  std::unique_ptr<ViewChangeManager> view_change_manager_;
  Stats* global_stats_;
  std::queue<std::pair<std::unique_ptr<Context>, std::unique_ptr<Request>>>
      request_pending_;
  std::mutex mutex_;
};

}  // namespace resdb
