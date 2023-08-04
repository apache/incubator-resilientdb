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

#include "absl/status/status.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/pow/miner.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"

namespace resdb {

// Manager all the blocks and mine the new blocks.
class BlockManager {
 public:
  BlockManager(const ResDBPoCConfig& config);
  virtual ~BlockManager() = default;
  // ================ mining a new block ============================
  // All the mining functions below are not thread safe.
  // They should be run in the same thread.

  // Setup a new mining block with the transactions in the user_request.
  // new block will be saved to 'new_mining_block_'.
  int SetNewMiningBlock(std::unique_ptr<BatchClientTransactions> user_request);

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
      std::unique_ptr<BatchClientTransactions> user_request);

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

  uint64_t first_block_time_ = 0, first_mine_time_ = 0;
  std::map<uint64_t, uint64_t> create_time_;
  std::atomic<uint64_t> last_update_time_;
};

}  // namespace resdb
