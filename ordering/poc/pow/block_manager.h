#pragma once

#include "absl/status/status.h"
#include "config/resdb_poc_config.h"
#include "ordering/poc/pow/miner.h"
#include "ordering/poc/proto/pow.pb.h"

namespace resdb {

// Manager all the blocks and mine the new blocks.
class BlockManager {
 public:
  BlockManager(const ResDBPoCConfig& config);

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
  absl::Status Mine();

  // Shift the slice.
  void SetSliceIdx(const SliceInfo& slice_info);

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

  // Obtain the slice shift of the current mining block.
  int32_t GetSliceIdx();

  // Update the miner info.
  void SetTargetValue(const HashValue& target_value);

 private:
  HashValue GetPreviousBlcokHash();
  int AddNewBlock(std::unique_ptr<Block> new_block);
  uint64_t GetCurrentHeightNoLock();

 private:
  std::mutex mtx_;
  std::unique_ptr<Miner> miner_;
  // Blocks that have been committed.
  // TODO move to executor and write to ds.
  std::vector<std::unique_ptr<Block> > block_list_ GUARDED_BY(mtx_);
  // The current minning block.
  std::unique_ptr<Block> new_mining_block_;
};

}  // namespace resdb
