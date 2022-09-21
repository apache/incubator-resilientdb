#include "ordering/poc/pow/block_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"
#include "crypto/signature_verifier.h"
#include "ordering/poc/pow/merkle.h"
#include "ordering/poc/pow/miner_utils.h"

namespace resdb {

BlockManager::BlockManager(const ResDBPoCConfig& config) {
  miner_ = std::make_unique<Miner>(config);
}

int BlockManager::SetNewMiningBlock(
    std::unique_ptr<BatchClientTransactions> client_request) {
  std::unique_ptr<Block> new_block = std::make_unique<Block>();
  if (client_request->min_seq() != GetLastSeq() + 1) {
    LOG(ERROR) << "seq invalid:" << client_request->min_seq()
               << " last seq:" << GetLastSeq();
    return -2;
  }

  new_block->mutable_header()->set_height(GetCurrentHeight() + 1);
  *new_block->mutable_header()->mutable_pre_hash() =
      GetPreviousBlcokHash();  // set the hash value of the parent block.
  client_request->SerializeToString(new_block->mutable_transaction_data());
  new_block->set_min_seq(client_request->min_seq());
  new_block->set_max_seq(client_request->max_seq());
  *new_block->mutable_header()->mutable_merkle_hash() =
      Merkle::MakeHash(*client_request);
  new_mining_block_ = std::move(new_block);
  miner_->SetSliceIdx(0);
  return 0;
}

Block* BlockManager::GetNewMiningBlock() {
  return new_mining_block_ == nullptr ? nullptr : new_mining_block_.get();
}

// Mine the nonce.
absl::Status BlockManager::Mine() {
  if (new_mining_block_ == nullptr) {
    LOG(ERROR) << "don't contain mining block.";
    return absl::InvalidArgumentError("height invalid");
  }

  if (new_mining_block_->header().height() != GetCurrentHeight() + 1) {
    // a new block has been committed.
    LOG(ERROR) << "new block height:" << new_mining_block_->header().height()
               << " current height:" << GetCurrentHeight() << " not equal";
    return absl::InvalidArgumentError("height invalid");
  }

  return miner_->Mine(new_mining_block_.get());
}

int32_t BlockManager::GetSliceIdx() { return miner_->GetSliceIdx(); }

void BlockManager::SetSliceIdx(const SliceInfo& slice_info) {
  if (new_mining_block_ == nullptr) {
    return;
  }
  if (slice_info.height() != new_mining_block_->header().height()) {
    return;
  }
  miner_->SetSliceIdx(slice_info.shift_idx());
}

int BlockManager::Commit() {
  int ret = AddNewBlock(std::move(new_mining_block_));
  new_mining_block_ = nullptr;
  return ret;
}

// =============== Mining Related End ==========================
int BlockManager::Commit(std::unique_ptr<Block> new_block) {
  return AddNewBlock(std::move(new_block));
}

int BlockManager::AddNewBlock(std::unique_ptr<Block> new_block) {
  {
    std::unique_lock<std::mutex> lck(mtx_);
    if (new_block->header().height() != GetCurrentHeightNoLock() + 1) {
      // a new block has been committed.
      LOG(ERROR) << "new block height:" << new_block->header().height()
                 << " current height:" << GetCurrentHeightNoLock()
                 << " not equal";
      return -2;
    }
    LOG(INFO) << "============= commit new block:"
              << new_block->header().height()
              << " current height:" << GetCurrentHeightNoLock();
    miner_->Terminate();
    block_list_.push_back(std::move(new_block));
  }
  return 0;
}

bool BlockManager::VerifyBlock(const Block* block) {
  if (!miner_->IsValidHash(block)) {
    LOG(ERROR) << "hash not valid:" << block->hash().DebugString();
    return false;
  }

  BatchClientTransactions client_request;
  if (!client_request.ParseFromString(block->transaction_data())) {
    LOG(ERROR) << "parse transaction fail";
    return false;
  }
  return Merkle::MakeHash(client_request) == block->header().merkle_hash();
}

uint64_t BlockManager::GetCurrentHeight() {
  std::unique_lock<std::mutex> lck(mtx_);
  return GetCurrentHeightNoLock();
}

uint64_t BlockManager::GetLastSeq() {
  std::unique_lock<std::mutex> lck(mtx_);
  return block_list_.empty() ? 0 : block_list_.back()->max_seq();
}

uint64_t BlockManager::GetCurrentHeightNoLock() {
  return block_list_.empty() ? 0 : block_list_.back()->header().height();
}

HashValue BlockManager::GetPreviousBlcokHash() {
  std::unique_lock<std::mutex> lck(mtx_);
  return block_list_.empty() ? HashValue() : block_list_.back()->hash();
}

Block* BlockManager::GetBlockByHeight(uint64_t height) {
  std::unique_lock<std::mutex> lck(mtx_);
  if (block_list_.empty() || block_list_.back()->header().height() < height) {
    return nullptr;
  }
  return block_list_[height - 1].get();
}

void BlockManager::SetTargetValue(const HashValue& target_value) {
  miner_->SetTargetValue(target_value);
}

}  // namespace resdb
