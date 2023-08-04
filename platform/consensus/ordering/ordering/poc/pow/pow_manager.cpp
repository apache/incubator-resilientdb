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

#include "platform/consensus/ordering/poc/pow/pow_manager.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace {

std::unique_ptr<Request> NewRequest(PoWRequest type,
                                    const ::google::protobuf::Message& message,
                                    int32_t sender) {
  auto new_request = std::make_unique<Request>();
  new_request->set_type(type);
  new_request->set_sender_id(sender);
  message.SerializeToString(new_request->mutable_data());
  return new_request;
}

}  // namespace

PoWManager::PoWManager(const ResDBPoCConfig& config,
                       ReplicaCommunicator* client)
    : config_(config), bc_client_(client) {
  Reset();
  is_stop_ = false;
}

PoWManager::~PoWManager() { Stop(); }

std::unique_ptr<TransactionAccessor> PoWManager::GetTransactionAccessor(
    const ResDBPoCConfig& config) {
  return std::make_unique<TransactionAccessor>(config);
}

std::unique_ptr<ShiftManager> PoWManager::GetShiftManager(
    const ResDBPoCConfig& config) {
  return std::make_unique<ShiftManager>(config);
}

std::unique_ptr<BlockManager> PoWManager::GetBlockManager(
    const ResDBPoCConfig& config) {
  return std::make_unique<BlockManager>(config);
}

void PoWManager::Commit(std::unique_ptr<Block> block) {
  std::unique_lock<std::mutex> lck(tx_mutex_);
  if (block_manager_->Commit(std::move(block)) == 0) {
    NotifyNextBlock();
  }
  LOG(INFO) << "commit block done";
}

void PoWManager::NotifyNextBlock() {
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.notify_one();
  if (current_status_ == GENERATE_NEW) {
    current_status_ = NEXT_NEWBLOCK;
  }
  LOG(INFO) << "notify new block:" << current_status_;
}

PoWManager::BlockStatus PoWManager::GetBlockStatus() { return current_status_; }

absl::Status PoWManager::WaitBlockDone() {
  int timeout_ms = config_.GetMiningTime();
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
               [&] { return current_status_ == NEXT_NEWBLOCK; });
  if (current_status_ == NEXT_NEWBLOCK) {
    return absl::OkStatus();
  }
  return absl::NotFoundError("No new transaction.");
}

void PoWManager::Reset() {
  transaction_accessor_ = GetTransactionAccessor(config_);
  shift_manager_ = GetShiftManager(config_);
  block_manager_ = GetBlockManager(config_);
}

void PoWManager::Start() {
  miner_thread_ = std::thread(&PoWManager::MiningProcess, this);
  gossip_thread_ = std::thread(&PoWManager::GossipProcess, this);
  is_stop_ = false;
}

void PoWManager::Stop() {
  is_stop_ = true;
  if (miner_thread_.joinable()) {
    miner_thread_.join();
  }
  if (gossip_thread_.joinable()) {
    gossip_thread_.join();
  }
}

bool PoWManager::IsRunning() { return !is_stop_; }

void PoWManager::AddShiftMsg(const SliceInfo& slice_info) {
  shift_manager_->AddSliceInfo(slice_info);
}

int PoWManager::GetShiftMsg(const SliceInfo& slice_info) {
  if (!shift_manager_->Check(slice_info)) {
    return -1;
  }
  LOG(INFO) << "slice info is ok:" << slice_info.DebugString();
  if (block_manager_->SetSliceIdx(slice_info) == 1) {
    return 1;
  }
  return 0;
}

int PoWManager::GetMiningTxn(MiningType type) {
  std::unique_lock<std::mutex> lck(tx_mutex_);
  LOG(INFO) << "get mining txn status:" << current_status_;
  if (current_status_ == NEXT_NEWBLOCK) {
    type = MiningType::NEWBLOCK;
  }
  if (type == NEWBLOCK) {
    uint64_t max_seq = std::max(block_manager_->GetLastSeq(),
                                block_manager_->GetLastCandidateSeq());
    LOG(ERROR) << "get block last max:" << block_manager_->GetLastSeq() << " "
               << block_manager_->GetLastCandidateSeq();
    auto client_tx = transaction_accessor_->ConsumeTransactions(max_seq + 1);
    if (client_tx == nullptr) {
      return -2;
    }
    block_manager_->SetNewMiningBlock(std::move(client_tx));
  } else {
    int ret = GetShiftMsg(need_slice_info_);
    if (ret == 1) {
      // no solution after enought shift.
      return 1;
    }
    if (ret != 0) {
      BroadCastShiftMsg(need_slice_info_);
      return -2;
    }
    LOG(INFO) << "get shift msg:" << need_slice_info_.DebugString();
  }
  return 0;
}

PoWManager::MiningStatus PoWManager::Wait() {
  current_status_ = GENERATE_NEW;
  auto mining_thread = std::thread([&]() {
    absl::Status status = block_manager_->Mine();
    if (status.ok()) {
      if (block_manager_->Commit() == 0) {
        NotifyNextBlock();
      }
    }
  });

  auto status = WaitBlockDone();
  if (mining_thread.joinable()) {
    mining_thread.join();
  }
  if (status.ok() || current_status_ == NEXT_NEWBLOCK) {
    return MiningStatus::OK;
  }
  return MiningStatus::TIMEOUT;
}

// receive a block before send and after need send
void PoWManager::SendShiftMsg() {
  SliceInfo slice_info;
  slice_info.set_shift_idx(block_manager_->GetSliceIdx() + 1);
  slice_info.set_height(block_manager_->GetNewMiningBlock()->header().height());
  slice_info.set_sender(config_.GetSelfInfo().id());
  BroadCastShiftMsg(slice_info);

  need_slice_info_ = slice_info;
}

int PoWManager::BroadCastNewBlock(const Block& block) {
  auto request = NewRequest(PoWRequest::TYPE_COMMITTED_BLOCK, block,
                            config_.GetSelfInfo().id());
  bc_client_->BroadCast(*request);
  return 0;
}

int PoWManager::BroadCastShiftMsg(const SliceInfo& slice_info) {
  auto request = NewRequest(PoWRequest::TYPE_SHIFT_MSG, slice_info,
                            config_.GetSelfInfo().id());
  bc_client_->BroadCast(*request);
  return 0;
}

// Broadcast the new block if once it is committed.
void PoWManager::GossipProcess() {
  uint64_t last_height = 0;
  while (IsRunning()) {
    std::unique_lock<std::mutex> lck(broad_cast_mtx_);
    broad_cast_cv_.wait_until(
        lck, std::chrono::system_clock::now() + std::chrono::seconds(1));

    Block* block = block_manager_->GetBlockByHeight(last_height + 1);
    if (block == nullptr) {
      continue;
    }
    BroadCastNewBlock(*block);
    last_height++;
  }
}

void PoWManager::NotifyBroadCast() {
  std::unique_lock<std::mutex> lck(broad_cast_mtx_);
  broad_cast_cv_.notify_all();
}

// Mining the new blocks got from PBFT cluster.
void PoWManager::MiningProcess() {
  MiningType type = MiningType::NEWBLOCK;
  while (IsRunning()) {
    int ret = GetMiningTxn(type);
    if (ret < 0) {
      usleep(10000);
      continue;
    } else if (ret > 0) {
      type = MiningType::NEWBLOCK;
      continue;
    }

    auto mining_status = Wait();
    if (mining_status == MiningStatus::TIMEOUT) {
      type = MiningType::SHIFTING;
      SendShiftMsg();
    } else {
      type = MiningType::NEWBLOCK;
      NotifyBroadCast();
    }
  }
}

}  // namespace resdb
