#pragma once

#include "common/queue/blocking_queue.h"
#include "config/resdb_poc_config.h"
#include "ordering/poc/pow/block_manager.h"
#include "ordering/poc/pow/miner_manager.h"
#include "ordering/poc/pow/transaction_accessor.h"
#include "ordering/poc/proto/pow.pb.h"
#include "server/consensus_service.h"

namespace resdb {

class ConsensusServicePoW : public ConsensusService {
 public:
  ConsensusServicePoW(const ResDBPoCConfig& config);
  virtual ~ConsensusServicePoW();

  // Start the service.
  void Start() override;
  void Stop() override;

  int ConsensusCommit(std::unique_ptr<Context> context,
                      std::unique_ptr<Request> request) override;

  std::vector<ReplicaInfo> GetReplicas() override;

  virtual int BroadCastNewBlock(const Block& block);
  virtual int BroadCastShiftMsg(const SliceInfo& slice_info);

 protected:
  virtual std::unique_ptr<BatchClientTransactions> GetClientTransactions();
  virtual std::unique_ptr<BatchClientTransactions> GetClientTransactions(
      uint64_t seq);

  // Mine new blocks.
  void MineNewBlock(std::unique_ptr<BatchClientTransactions> client_request);
  // Receive blocks committed from other replicas.
  int ProcessCommittedBlock(std::unique_ptr<Block> block);
  // Receive shlice shift.
  int ProcessShiftMsg(std::unique_ptr<SliceInfo> slice_info);

  // notify GossipProcess to broadcast the new minning block.
  void NotifyBroadCast();

  // Wait 2f+1 shift msg;
  void WaitShiftACK(const SliceInfo& slice_info);
  // Set up the height the shift msg wanted.
  bool SetCurrentShiftHeight(const uint64_t height);

  // Process running in threads.
  void MiningProcess();
  void GossipProcess();

 protected:
  std::unique_ptr<BlockManager> block_manager_;
  std::unique_ptr<MinerManager> miner_manager_;
  std::unique_ptr<TransactionAccessor> transaction_accessor_;
  std::thread miner_thread_, gossip_thread_;

  // Shift messages collector.
  std::mutex broad_cast_mtx_, shift_mtx_;
  std::condition_variable broad_cast_cv_, shift_cv_;
  // key by height and shift index.
  std::map<std::pair<uint64_t, int32_t>, std::set<int32_t>>
      shift_message_collector_ GUARDED_BY(mtx_);
  SliceInfo current_shift_ GUARDED_BY(shift_mtx_);
};

}  // namespace resdb
