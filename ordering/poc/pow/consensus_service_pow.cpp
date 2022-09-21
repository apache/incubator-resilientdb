#include "ordering/poc/pow/consensus_service_pow.h"

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

ConsensusServicePoW::ConsensusServicePoW(const ResDBPoCConfig& config)
    : ConsensusService(config) {
  block_manager_ = std::make_unique<BlockManager>(config);
  transaction_accessor_ = std::make_unique<TransactionAccessor>(config);
  miner_manager_ = std::make_unique<MinerManager>(config);
}

void ConsensusServicePoW::Start() {
  ConsensusService::Start();
  miner_thread_ = std::thread(&ConsensusServicePoW::MiningProcess, this);
  gossip_thread_ = std::thread(&ConsensusServicePoW::GossipProcess, this);
}

void ConsensusServicePoW::Stop() {
  ConsensusService::Stop();
  if (miner_thread_.joinable()) {
    miner_thread_.join();
  }

  if (gossip_thread_.joinable()) {
    NotifyBroadCast();
    gossip_thread_.join();
  }
}

ConsensusServicePoW::~ConsensusServicePoW() {
  if (miner_thread_.joinable()) {
    miner_thread_.join();
  }

  if (gossip_thread_.joinable()) {
    NotifyBroadCast();
    gossip_thread_.join();
  }
}

std::vector<ReplicaInfo> ConsensusServicePoW::GetReplicas() {
  return miner_manager_->GetReplicas();
}

// The implementation of PBFT.
int ConsensusServicePoW::ConsensusCommit(std::unique_ptr<Context> context,
                                         std::unique_ptr<Request> request) {
  // LOG(ERROR) << "recv impl type:" << request->type() << " "
  //          << request->client_info().DebugString()
  //         << "sender id:" << request->sender_id();
  switch (request->type()) {
    case PoWRequest::TYPE_COMMITTED_BLOCK: {
      std::unique_ptr<Block> block = std::make_unique<Block>();
      if (block->ParseFromString(request->data())) {
        return ProcessCommittedBlock(std::move(block));
      }
      break;
    }
    case PoWRequest::TYPE_SHIFT_MSG: {
      std::unique_ptr<SliceInfo> slice_info = std::make_unique<SliceInfo>();
      if (slice_info->ParseFromString(request->data())) {
        return ProcessShiftMsg(std::move(slice_info));
      }
      break;
    }
  }
  return 0;
}

// Query the requests with the seq equals to QueryRequest.seq()+1
// from PBFT cluster
std::unique_ptr<BatchClientTransactions>
ConsensusServicePoW::GetClientTransactions() {
  uint64_t max_seq = block_manager_->GetLastSeq();
  return GetClientTransactions(max_seq + 1);
}

std::unique_ptr<BatchClientTransactions>
ConsensusServicePoW::GetClientTransactions(uint64_t seq) {
  return transaction_accessor_->ConsumeTransactions(seq);
}

int ConsensusServicePoW::BroadCastNewBlock(const Block& block) {
  auto request = NewRequest(PoWRequest::TYPE_COMMITTED_BLOCK, block,
                            config_.GetSelfInfo().id());
  BroadCast(*request);
  return 0;
}

int ConsensusServicePoW::BroadCastShiftMsg(const SliceInfo& slice_info) {
  auto request = NewRequest(PoWRequest::TYPE_SHIFT_MSG, slice_info,
                            config_.GetSelfInfo().id());
  BroadCast(*request);
  return 0;
}

// Broadcast the new block if once it is committed.
void ConsensusServicePoW::GossipProcess() {
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

// Mining the new blocks got from PBFT cluster.
void ConsensusServicePoW::MiningProcess() {
  while (IsRunning()) {
    // Get the transactions.
    std::unique_ptr<BatchClientTransactions> client_request =
        GetClientTransactions();
    if (client_request == nullptr) {
      LOG(ERROR) << "no client request";
      sleep(1);
      continue;
    }
    MineNewBlock(std::move(client_request));
  }
}

void ConsensusServicePoW::NotifyBroadCast() {
  std::unique_lock<std::mutex> lck(broad_cast_mtx_);
  broad_cast_cv_.notify_all();
}

void ConsensusServicePoW::MineNewBlock(
    std::unique_ptr<BatchClientTransactions> client_request) {
  LOG(ERROR) << "========= mine new block, min seq:"
             << client_request->min_seq()
             << " max seq:" << client_request->max_seq()
             << " service id:" << config_.GetSelfInfo().id();
  // Set the new block.
  uint64_t seq = client_request->min_seq();
  block_manager_->SetNewMiningBlock(std::move(client_request));
  uint64_t start_time = get_sys_clock();
  // Keey mining until find out or receive a solution.
  while (IsRunning()) {
    // Mine the nonce.
    absl::Status status = block_manager_->Mine();
    if (status.ok()) {
      block_manager_->Commit();
      uint64_t end_time = get_sys_clock();
      LOG(ERROR) << " mine block successful, id:" << config_.GetSelfInfo().id()
                 << " seq:" << seq << " minging time:" << end_time - start_time;
      // Success. Then broadcast the solution.
      NotifyBroadCast();
      return;
    } else if (status.code() == absl::StatusCode::kCancelled) {
      NotifyBroadCast();
      return;
    } else if (status.code() == absl::StatusCode::kNotFound ||
               status.code() == absl::StatusCode::kDeadlineExceeded) {
      // start next round.
      LOG(ERROR) << "no result, start next round and do shift";
      // Shift the slice and wait the ack.
      SliceInfo slice_info;
      slice_info.set_shift_idx(block_manager_->GetSliceIdx());
      slice_info.set_height(
          block_manager_->GetNewMiningBlock()->header().height());
      slice_info.set_sender(config_.GetSelfInfo().id());
      WaitShiftACK(slice_info);
      continue;
    } else {
      LOG(ERROR) << "mining block fail, service id:"
                 << config_.GetSelfInfo().id();
      return;
    }
  }
}

// Receive block committed by other replicas.
int ConsensusServicePoW::ProcessCommittedBlock(std::unique_ptr<Block> block) {
  // Verify the block:
  if (!block_manager_->VerifyBlock(block.get())) {
    LOG(ERROR) << "block invalid.";
    return -2;
  }
  int ret = block_manager_->Commit(std::move(block));
  if (ret == 0) {
    // Receive a new committed block, broad cast to others.
    NotifyBroadCast();
  }
  return ret;
}

// ============== Shift =========================
// Wait for 2f+1 shift msg from others.
// If timeout, resend the shift message.
// If receive a commited block during waiting for the shift messages,
// end waiting.
void ConsensusServicePoW::WaitShiftACK(const SliceInfo& slice_info) {
  while (IsRunning()) {
    BroadCastShiftMsg(slice_info);
    std::unique_lock<std::mutex> lck(shift_mtx_);
    // Only wait for the shift messages with the same height.
    // when received 2f+1 same shift messages with the same height and shift
    // idx, where the shift idx is larger than the one in slice_info, stop
    // waiting and update the slice.
    if (shift_cv_.wait_for(lck, std::chrono::seconds(1), [&] {
          return slice_info.height() == current_shift_.height() &&
                 slice_info.shift_idx() <= current_shift_.shift_idx();
        })) {
      SliceInfo new_slice_info = current_shift_;
      new_slice_info.set_shift_idx(current_shift_.shift_idx() + 1);
      assert(new_slice_info.shift_idx() <= 5);
      block_manager_->SetSliceIdx(new_slice_info);
      return;
    } else {
      // Receive committed message while waiting for the shift messages.
      if (slice_info.height() <= block_manager_->GetCurrentHeight()) {
        // The block has been committed.
        LOG(ERROR) << " block has been committed, height:"
                   << slice_info.height()
                   << " current height:" << block_manager_->GetCurrentHeight();
        return;
      }
    }
  }
}

int ConsensusServicePoW::ProcessShiftMsg(
    std::unique_ptr<SliceInfo> slice_info) {
  std::unique_lock<std::mutex> lck(shift_mtx_);
  LOG(ERROR) << "receive slice info:" << slice_info->DebugString()
             << " id:" << config_.GetSelfInfo().id();

  if (slice_info->height() < current_shift_.height() ||
      (slice_info->height() == current_shift_.height() &&
       slice_info->shift_idx() < current_shift_.shift_idx())) {
    LOG(ERROR) << "shift info is old:" << slice_info->DebugString()
               << " current:" << current_shift_.DebugString();
    return 0;
  }
  auto key = std::make_pair(slice_info->height(), slice_info->shift_idx());
  shift_message_collector_[key].insert(slice_info->sender());
  LOG(ERROR) << " ====================     !!! receie slice info size:"
             << shift_message_collector_[key].size()
             << " id:" << config_.GetSelfInfo().id();
  size_t n = config_.GetReplicaNum();
  size_t f = config_.GetMaxMaliciousReplicaNum();
  // receive n-f shift message, shift to the next slice.
  if (shift_message_collector_[key].size() >= n - f) {
    current_shift_ = *slice_info;
    // Set a new shift.
    current_shift_.set_shift_idx(current_shift_.shift_idx());
    LOG(INFO) << "====================== get new shift slice:"
              << current_shift_.DebugString();
    while (!shift_message_collector_.empty() &&
           shift_message_collector_.begin()->first <= key) {
      shift_message_collector_.erase(shift_message_collector_.begin());
    }
    shift_cv_.notify_all();
  }
  return 0;
}

}  // namespace resdb
