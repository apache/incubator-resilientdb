#include "ordering/pbft/transaction_manager.h"

#include <glog/logging.h>

namespace resdb {

TransactionManager::TransactionManager(
    const ResDBConfig& config,
    std::unique_ptr<TransactionExecutorImpl> executor_impl,
    CheckPointManager* checkpoint_manager, SystemInfo* system_info)
    : config_(config),
      queue_("executed"),
      txn_db_(checkpoint_manager->GetTxnDB()),
      system_info_(system_info),
      checkpoint_manager_(checkpoint_manager),
      transaction_executor_(std::make_unique<TransactionExecutor>(
          config,
          [&](std::unique_ptr<Request> request,
              std::unique_ptr<BatchClientResponse> resp_msg) {
            uint64_t seq = request->seq();
            resp_msg->set_proxy_id(request->proxy_id());
            resp_msg->set_seq(request->seq());
            resp_msg->set_current_view(request->current_view());
            queue_.Push(std::move(resp_msg));
            if (checkpoint_manager_) {
              checkpoint_manager_->AddCommitData(std::move(request));
            }
            collector_pool_->Update(seq);
          },
          system_info_, std::move(executor_impl))),
      collector_pool_(std::make_unique<LockFreeCollectorPool>(
          "txn", config_.GetMaxProcessTxn(), transaction_executor_.get(),
          config_.GetConfigData().enable_viewchange())) {
  global_stats_ = Stats::GetGlobalStats();
}

std::unique_ptr<BatchClientResponse> TransactionManager::GetResponseMsg() {
  return queue_.Pop();
}

int64_t TransactionManager::GetCurrentPrimary() const {
  return system_info_->GetPrimaryId();
}

uint64_t TransactionManager ::GetCurrentView() const {
  return system_info_->GetCurrentView();
}

void TransactionManager::SetNextSeq(uint64_t seq) { next_seq_ = seq; }

absl::StatusOr<uint64_t> TransactionManager::AssignNextSeq() {
  std::unique_lock<std::mutex> lk(seq_mutex_);
  uint32_t max_executed_seq = transaction_executor_->GetMaxPendingExecutedSeq();
  global_stats_->SeqGap(next_seq_ - max_executed_seq);
  if (next_seq_ - max_executed_seq >
      static_cast<uint64_t>(config_.GetMaxProcessTxn())) {
    return absl::InvalidArgumentError("Seq has been used up.");
  }
  return next_seq_++;
}

std::vector<ReplicaInfo> TransactionManager::GetReplicas() {
  return system_info_->GetReplicas();
}

// Check if the request is valid.
// 1. view is the same as the current view
// 2. seq is larger or equal than the next execute seq.
// 3. inside the water mark.
bool TransactionManager::IsValidMsg(const Request& request) {
  if (request.type() == Request::TYPE_RESPONSE) {
    return true;
  }
  // view should be the same as the current one.
  if (static_cast<uint64_t>(request.current_view()) != GetCurrentView()) {
    LOG(ERROR) << "message view :[" << request.current_view()
               << "] is older than the cur view :[" << GetCurrentView() << "]";
    return false;
  }

  if (static_cast<uint64_t>(request.seq()) <
      transaction_executor_->GetMaxPendingExecutedSeq()) {
    return false;
  }

  return true;
}

bool TransactionManager::MayConsensusChangeStatus(
    int type, int received_count, std::atomic<TransactionStatue>* status) {
  switch (type) {
    case Request::TYPE_PRE_PREPARE:
      if (*status == TransactionStatue::None) {
        TransactionStatue old_status = TransactionStatue::None;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_PREPARE,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
    case Request::TYPE_PREPARE:
      if (*status == TransactionStatue::READY_PREPARE &&
          config_.GetMinDataReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::READY_PREPARE;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_COMMIT,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
      }
      break;
    case Request::TYPE_COMMIT:
      if (*status == TransactionStatue::READY_COMMIT &&
          config_.GetMinDataReceiveNum() <= received_count) {
        TransactionStatue old_status = TransactionStatue::READY_COMMIT;
        return status->compare_exchange_strong(
            old_status, TransactionStatue::READY_EXECUTE,
            std::memory_order_acq_rel, std::memory_order_acq_rel);
        return true;
      }
      break;
  }
  return false;
}

// Add commit messages and return the number of messages have been received.
// The commit messages only include post(pre-prepare), prepare and commit
// messages. Messages are handled by state (PREPARE,COMMIT,READY_EXECUTE).

// If there are enough messages and the state is changed after adding the
// message, return 1, otherwise return 0. Return -2 if the request is not valid.
CollectorResultCode TransactionManager::AddConsensusMsg(
    const SignatureInfo& signature, std::unique_ptr<Request> request) {
  if (request == nullptr || !IsValidMsg(*request)) {
    return CollectorResultCode::INVALID;
  }
  int type = request->type();
  uint64_t seq = request->seq();
  int resp_received_count = 0;

  int ret = collector_pool_->GetCollector(seq)->AddRequest(
      std::move(request), signature, type == Request::TYPE_PRE_PREPARE,
      [&](const Request& request, int received_count,
          TransactionCollector::CollectorDataType* data,
          std::atomic<TransactionStatue>* status) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          resp_received_count = 1;
        }
      });
  if (ret != 0) {
    return CollectorResultCode::INVALID;
  }
  if (resp_received_count > 0) {
    return CollectorResultCode::STATE_CHANGED;
  }
  return CollectorResultCode::OK;
}

// Save the committed request with its 2f+1 proof.
void TransactionManager::SaveCommittedRequest(
    const Request& request,
    TransactionCollector::CollectorDataType* proof_data) {
  if (!config_.IsCheckPointEnabled()) {
    return;
  }
}

int TransactionManager::SaveCommittedRequest(
    const RequestWithProof& proof_data) {
  uint64_t seq = proof_data.seq();
  if (committed_data_.find(seq) != committed_data_.end()) {
    LOG(ERROR) << "seq data:" << seq << "  has been set";
    return -2;
  }
  std::unique_lock<std::mutex> lk(data_mutex_);
  committed_proof_[seq].clear();
  for (auto& data : proof_data.proofs()) {
    std::unique_ptr<RequestInfo> info = std::make_unique<RequestInfo>();
    info->request = std::make_unique<Request>(data.request());
    info->signature = data.signature();
    committed_proof_[seq].push_back(std::move(info));
  }
  committed_data_[seq] = proof_data.request();
  LOG(INFO) << "add committed data:" << seq
            << " size:" << committed_proof_[seq].size();
  return 0;
}

RequestSet TransactionManager::GetRequestSet(uint64_t min_seq,
                                             uint64_t max_seq) {
  RequestSet ret;
  std::unique_lock<std::mutex> lk(data_mutex_);
  for (uint64_t i = min_seq; i <= max_seq; ++i) {
    if (committed_data_.find(i) == committed_data_.end()) {
      LOG(ERROR) << "seq :" << i << " doesn't exist";
      continue;
    }
    RequestWithProof* request = ret.add_requests();
    *request->mutable_request() = committed_data_[i];
    request->set_seq(i);
    for (const auto& request_info : committed_proof_[i]) {
      RequestWithProof::RequestData* data = request->add_proofs();
      *data->mutable_request() = *request_info->request;
      *data->mutable_signature() = request_info->signature;
    }
  }
  return ret;
}

// Get the transactions that have been execuited.
Request* TransactionManager::GetRequest(uint64_t seq) {
  return txn_db_->Get(seq);
}

std::vector<RequestInfo> TransactionManager::GetPreparedProof(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetPreparedProof();
}

TransactionStatue TransactionManager::GetTransactionState(uint64_t seq) {
  return collector_pool_->GetCollector(seq)->GetStatus();
}

int TransactionManager::GetReplicaState(ReplicaState* state) {
  state->set_view(GetCurrentView());
  *state->mutable_replica_info() = config_.GetSelfInfo();
  return 0;
}

}  // namespace resdb
