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

#include "platform/consensus/ordering/poc/pow/block_manager.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"
#include "platform/consensus/ordering/poc/pow/merkle.h"
#include "platform/consensus/ordering/poc/pow/miner_utils.h"
#include "platform/proto/resdb.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {

BlockManager::BlockManager(const ResDBPoCConfig& config) : config_(config) {
  miner_ = std::make_unique<Miner>(config);
  last_update_time_ = 0;
}

void BlockManager::SaveClientTransactions(
    std::unique_ptr<BatchClientTransactions> user_request) {
  for (const ClientTransactions& client_tx : user_request->transactions()) {
    *request_candidate_.add_transactions() = client_tx;
  }
  if (request_candidate_.min_seq() == 0) {
    request_candidate_.set_min_seq(user_request->min_seq());
  } else {
    request_candidate_.set_min_seq(
        std::min(request_candidate_.min_seq(), user_request->min_seq()));
  }
  request_candidate_.set_max_seq(
      std::max(request_candidate_.max_seq(), user_request->max_seq()));
}

int BlockManager::SetNewMiningBlock(
    std::unique_ptr<BatchClientTransactions> user_request) {
  SaveClientTransactions(std::move(user_request));

  std::unique_ptr<Block> new_block = std::make_unique<Block>();
  if (request_candidate_.min_seq() != GetLastSeq() + 1) {
    LOG(ERROR) << "seq invalid:" << request_candidate_.min_seq()
               << " last seq:" << GetLastSeq();
    return -2;
  }

  int64_t new_time = 0;
  for (ClientTransactions& client_tx :
       *request_candidate_.mutable_transactions()) {
    new_time = client_tx.create_time();
    if (new_time > 0) {
      create_time_[client_tx.seq()] = new_time;
      client_tx.clear_create_time();
    }
  }

  new_block->mutable_header()->set_height(GetCurrentHeight() + 1);
  *new_block->mutable_header()->mutable_pre_hash() =
      GetPreviousBlcokHash();  // set the hash value of the parent block.
  request_candidate_.SerializeToString(new_block->mutable_transaction_data());
  new_block->set_min_seq(request_candidate_.min_seq());
  new_block->set_max_seq(request_candidate_.max_seq());
  *new_block->mutable_header()->mutable_merkle_hash() =
      Merkle::MakeHash(request_candidate_);
  new_block->set_miner(config_.GetSelfInfo().id());
  new_block->set_block_time(GetCurrentTime());

  LOG(ERROR) << "create new block:"
             << request_candidate_.transactions(0).create_time() << "["
             << new_block->min_seq() << "," << new_block->max_seq() << "]"
             << " miner:" << config_.GetSelfInfo().id()
             << " time:" << new_block->block_time()
             << " delay:" << (new_block->block_time() - new_time) / 1000000.0
             << " current:" << GetCurrentTime();
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

int BlockManager::SetSliceIdx(const SliceInfo& slice_info) {
  if (new_mining_block_ == nullptr) {
    return 0;
  }
  if (slice_info.height() != new_mining_block_->header().height()) {
    return -2;
  }
  size_t f = config_.GetMaxMaliciousReplicaNum();
  LOG(ERROR) << "set slice idx, current idx:" << slice_info.shift_idx()
             << " f:" << f;
  if (slice_info.shift_idx() > f) {
    // reset
    miner_->SetSliceIdx(0);
    return 1;
  }
  miner_->SetSliceIdx(slice_info.shift_idx());
  return 0;
}

int BlockManager::Commit() {
  request_candidate_.Clear();
  uint64_t mining_time = GetCurrentTime() - new_mining_block_->block_time();
  new_mining_block_->set_mining_time(mining_time);
  int ret = AddNewBlock(std::move(new_mining_block_));
  new_mining_block_ = nullptr;
  return ret;
}

// =============== Mining Related End ==========================
int BlockManager::Commit(std::unique_ptr<Block> new_block) {
  return AddNewBlock(std::move(new_block));
}

int BlockManager::AddNewBlock(std::unique_ptr<Block> new_block) {
  std::unique_lock<std::mutex> lck(mtx_);
  if (new_block->header().height() != GetCurrentHeightNoLock() + 1) {
    // a new block has been committed.
    LOG(ERROR) << "new block height:" << new_block->header().height()
               << " current height:" << GetCurrentHeightNoLock()
               << " not equal";
    return -2;
  }
  LOG(INFO) << "============= commit new block:" << new_block->header().height()
            << " current height:" << GetCurrentHeightNoLock();
  LOG(INFO) << "commit:" << new_block->header().height()
            << " from:" << new_block->miner()
            << " mining time:" << new_block->mining_time() / 1000000.0;
  miner_->Terminate();
  request_candidate_.Clear();
  block_list_.push_back(std::move(new_block));
  Execute(*block_list_.back());
  return 0;
}

void BlockManager::Execute(const Block& block) {
  BatchClientTransactions batch_user_request;
  if (!batch_user_request.ParseFromString(block.transaction_data())) {
    LOG(ERROR) << "parse client transaction fail";
  }

  LOG(INFO) << " execute seq:[" << batch_user_request.min_seq() << ","
            << batch_user_request.max_seq() << "]";
  uint64_t lat = 0;
  int num = 0;
  uint64_t total_tx = 0;
  uint64_t run_time = GetCurrentTime();
  for (const ClientTransactions& client_tx :
       batch_user_request.transactions()) {
    BatchUserRequest batch_request;
    if (!batch_request.ParseFromString(client_tx.transaction_data())) {
      LOG(ERROR) << "parse data fail";
    }
    total_tx += batch_request.user_requests_size();

    if (block.miner() == config_.GetSelfInfo().id()) {
      uint64_t create_time = create_time_[client_tx.seq()];
      uint64_t latency = run_time - create_time;
      lat += latency;
      num++;
    }
  }

  if (total_tx > 0) {
    uint64_t current_time = GetCurrentTime();
    if (last_update_time_ > 0) {
      LOG(ERROR) << " tps:"
                 << total_tx / ((current_time - last_update_time_) / 1000000.0)
                 << " has waited:"
                 << ((current_time - last_update_time_) / 1000000.0);
    }
    last_update_time_ = current_time;
  }
  if (lat > 0) {
    LOG(ERROR) << " execute seq:[" << batch_user_request.min_seq() << ","
               << batch_user_request.max_seq() << "]:"
               << " total:" << total_tx << " lat:" << (lat / 1000000.0 / num);
  } else {
    LOG(ERROR) << " execute seq:[" << batch_user_request.min_seq() << ","
               << batch_user_request.max_seq()
               << "]:"
                  " total:"
               << total_tx;
  }
}

bool BlockManager::VerifyBlock(const Block* block) {
  if (!miner_->IsValidHash(block)) {
    LOG(ERROR) << "hash not valid:" << block->hash().DebugString();
    return false;
  }

  BatchClientTransactions user_request;
  if (!user_request.ParseFromString(block->transaction_data())) {
    LOG(ERROR) << "parse transaction fail";
    return false;
  }
  return Merkle::MakeHash(user_request) == block->header().merkle_hash();
}

uint64_t BlockManager::GetCurrentHeight() {
  std::unique_lock<std::mutex> lck(mtx_);
  return GetCurrentHeightNoLock();
}

uint64_t BlockManager::GetLastSeq() {
  std::unique_lock<std::mutex> lck(mtx_);
  return block_list_.empty() ? 0 : block_list_.back()->max_seq();
}

uint64_t BlockManager::GetLastCandidateSeq() {
  return request_candidate_.max_seq();
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
