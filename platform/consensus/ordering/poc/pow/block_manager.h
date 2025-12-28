#pragma once

#include "absl/status/status.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/pow/miner.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

// Manager all the blocks and mine the new blocks.
class BlockManager {
 public:
  BlockManager(const ResDBPoCConfig& config);
  virtual ~BlockManager() = default;
  // ================ mining a new block ============================
  // All the mining functions below are not thread safe.
  // They should be run in the same thread.

  // Setup a new mining block with the transactions in the client_request.
  // new block will be saved to 'new_mining_block_'.
  int SetNewMiningBlock(
      std::unique_ptr<BatchClientTransactions> client_request);

  // Get the new mining block set from SetMiningBlock().
  Block* GetNewMiningBlock();

  // Mine the nonce using the local slice.
  virtual absl::Status Mine();

  // Shift the slice.
  virtual int SetSliceIdx(const SliceInfo& slice_info);

  // Commit the new mining block.
  int Commit();

  // ===============================================================

  // Commit a new block other than the new mining block.
  // The new mining block should call the Commit() to commit.
  int Commit(std::unique_ptr<Block> new_block);

  // 1 verify the hash value
  // 2 verify the transaction
  bool VerifyBlock(const Block* block);

  // Obtain the block with height 'height'.
  Block* GetBlockByHeight(uint64_t height);

  // Get the height of committed block with max height.
  uint64_t GetCurrentHeight();
  uint64_t GetLastSeq();
  uint64_t GetLastCandidateSeq();

  // Obtain the slice shift of the current mining block.
  int32_t GetSliceIdx();

  // Update the miner info.
  void SetTargetValue(const HashValue& target_value);

 private:
  HashValue GetPreviousBlcokHash();
  int AddNewBlock(std::unique_ptr<Block> new_block);
  uint64_t GetCurrentHeightNoLock();
  void Execute(const Block& block);
  void SaveClientTransactions(
      std::unique_ptr<BatchClientTransactions> client_request);

 private:
  ResDBPoCConfig config_;
  std::mutex mtx_;
  std::unique_ptr<Miner> miner_;
  // Blocks that have been committed.
  // TODO move to executor and write to ds.
  std::vector<std::unique_ptr<Block> > block_list_ GUARDED_BY(mtx_);
  // The current minning block.
  std::unique_ptr<Block> new_mining_block_;
  BatchClientTransactions request_candidate_;

  Stats* global_stats_;
  uint64_t first_block_time_ = 0, first_mine_time_ = 0;
  std::map<uint64_t, uint64_t> create_time_;
  std::atomic<uint64_t> last_update_time_;
  PrometheusHandler* prometheus_handler_;
};

}  // namespace resdb
