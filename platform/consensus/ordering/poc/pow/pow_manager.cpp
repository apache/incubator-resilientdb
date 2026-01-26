/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/consensus/ordering/poc/pow/pow_manager.h"

#include "common/utils/utils.h"
#include "glog/logging.h"

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
  // prometheus_handler_ = Stats::GetGlobalPrometheus();
}

PoWManager::~PoWManager() {
  Stop();
  if (miner_thread_.joinable()) {
    miner_thread_.join();
  }
  if (gossip_thread_.joinable()) {
    gossip_thread_.join();
  }
}

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
    LOG(ERROR) << "commit block succ";
    NotifyNextBlock();
  }
  LOG(ERROR) << "commit block done";
}

void PoWManager::NotifyNextBlock() {
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.notify_one();
  if (current_status_ == GENERATE_NEW) {
    current_status_ = NEXT_NEWBLOCK;
  }
  LOG(ERROR) << "notify block:" << current_status_;
}

PoWManager::BlockStatus PoWManager::GetBlockStatus() { return current_status_; }

absl::Status PoWManager::WaitBlockDone() {
  LOG(ERROR) << "wait block:" << current_status_;
  int timeout_ms = config_.GetMiningTime();
  // 60000000;
  // int timeout_ms = 300000000;
  std::unique_lock<std::mutex> lk(mutex_);
  cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
               [&] { return current_status_ == NEXT_NEWBLOCK; });
  LOG(ERROR) << "wait block done:" << current_status_;
  if (current_status_ == NEXT_NEWBLOCK) {
    return absl::OkStatus();
  }
  return absl::NotFoundError("No new transaction.");
}

void PoWManager::Reset() {
  transaction_accessor_ = GetTransactionAccessor(config_);
  shift_manager_ = GetShiftManager(config_);
  block_manager_ = GetBlockManager(config_);
  LOG(ERROR) << "reset:" << transaction_accessor_.get();
}

void PoWManager::Start() {
  miner_thread_ = std::thread(&PoWManager::MiningProcess, this);
  gossip_thread_ = std::thread(&PoWManager::GossipProcess, this);
  is_stop_ = false;
}

void PoWManager::Stop() { is_stop_ = true; }

bool PoWManager::IsRunning() { return !is_stop_; }

void PoWManager::AddShiftMsg(const SliceInfo& slice_info) {
  shift_manager_->AddSliceInfo(slice_info);
}

int PoWManager::GetShiftMsg(const SliceInfo& slice_info) {
  LOG(ERROR) << "check shift msg:" << slice_info.DebugString();
  if (!shift_manager_->Check(slice_info)) {
    return -1;
  }
  LOG(ERROR) << "slice info is ok:" << slice_info.DebugString();
  if (block_manager_->SetSliceIdx(slice_info) == 1) {
    return 1;
  }
  return 0;
}

int PoWManager::GetMiningTxn(MiningType type) {
  std::unique_lock<std::mutex> lck(tx_mutex_);
  LOG(ERROR) << "get mining txn status:" << current_status_;
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
    // prometheus_handler_->Inc(SHIFT_MSG, 1);
    int ret = GetShiftMsg(need_slice_info_);
    LOG(ERROR) << "get shift msg ret:" << ret;
    if (ret == 1) {
      // no solution after enought shift.
      return 1;
    }
    if (ret != 0) {
      BroadCastShiftMsg(need_slice_info_);
      return -2;
    }
    LOG(ERROR) << "get shift msg:" << need_slice_info_.DebugString();
  }
  return 0;
}

PoWManager::MiningStatus PoWManager::Wait() {
  current_status_ = GENERATE_NEW;
  auto mining_thread = std::thread([&]() {
    LOG(ERROR) << "mine";
    absl::Status status = block_manager_->Mine();
    if (status.ok()) {
      if (block_manager_->Commit() == 0) {
        NotifyNextBlock();
      }
      LOG(ERROR) << "done mine";
    }
    LOG(ERROR) << "mine:" << status.ok();
  });

  auto status = WaitBlockDone();
  if (mining_thread.joinable()) {
    mining_thread.join();
  }
  LOG(ERROR) << "success:" << status.ok() << " status:" << current_status_;
  if (status.ok() || current_status_ == NEXT_NEWBLOCK) {
    return MiningStatus::OK;
  }
  return MiningStatus::TIMEOUT;
}

// receive a block before send and after need send
void PoWManager::SendShiftMsg() {
  LOG(ERROR) << "send shift";
  SliceInfo slice_info;
  slice_info.set_shift_idx(block_manager_->GetSliceIdx() + 1);
  slice_info.set_height(block_manager_->GetNewMiningBlock()->header().height());
  slice_info.set_sender(config_.GetSelfInfo().id());
  BroadCastShiftMsg(slice_info);

  need_slice_info_ = slice_info;
  LOG(ERROR) << "send shift msg";
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
      // LOG(ERROR) << "======= get block fail:" << last_height
      //          << " id:" << config_.GetSelfInfo().id();
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
  if (config_.GetSelfInfo().id() > 8 && config_.GetSelfInfo().id() < 13) {
    // return;
  }
  LOG(ERROR) << "start";
  MiningType type = MiningType::NEWBLOCK;
  while (IsRunning()) {
    int ret = GetMiningTxn(type);
    LOG(ERROR) << "get mining ret:" << ret;
    if (ret < 0) {
      usleep(10000);
      continue;
    } else if (ret > 0) {
      LOG(ERROR) << "get new block";
      type = MiningType::NEWBLOCK;
      continue;
    }
    LOG(ERROR) << "get ok";

    auto mining_status = Wait();
    LOG(ERROR) << "done:" << mining_status;
    if (mining_status == MiningStatus::TIMEOUT) {
      type = MiningType::SHIFTING;
      SendShiftMsg();
    } else {
      type = MiningType::NEWBLOCK;
      LOG(ERROR) << "done";
      NotifyBroadCast();
    }
  }
}

}  // namespace resdb
