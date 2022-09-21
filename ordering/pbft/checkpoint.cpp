#include "ordering/pbft/checkpoint.h"

#include "glog/logging.h"
#include "ordering/pbft/transaction_utils.h"

namespace resdb {

CheckPoint::CheckPoint(const ResDBConfig& config,
                       ResDBReplicaClient* replica_client)
    : config_(config),
      replica_client_(replica_client),
      checkpoint_info_(std::make_unique<CheckPointInfo>(config)),
      checkpoint_collector_(std::make_unique<CheckPointCollector>(
          config, checkpoint_info_.get())),
      stop_(false) {
  if (config_.IsCheckPointEnabled()) {
    checkpoint_thread_ = std::thread(&CheckPoint::UpdateCheckPointStatus, this);
  }
}

CheckPoint::~CheckPoint() {
  stop_ = true;
  if (checkpoint_thread_.joinable()) {
    checkpoint_thread_.join();
  }
}

CheckPointInfo* CheckPoint::GetCheckPointInfo() {
  if (!config_.IsCheckPointEnabled()) {
    return nullptr;
  }
  return checkpoint_info_.get();
}

int CheckPoint::ProcessCheckPoint(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  return checkpoint_collector_->AddCheckPointMsg(context->signature,
                                                 std::move(request));
}

void CheckPoint::UpdateCheckPointStatus() {
  uint64_t last_seq = 0;
  while (!stop_) {
    CheckPointData checkpoint_data = checkpoint_info_->GetCheckPointData();
    if (last_seq == checkpoint_data.seq()) {
      LOG(ERROR) << "no new checkpoint:" << last_seq;
      sleep(1);
      continue;
    }
    std::unique_ptr<Request> checkpoint_request = NewRequest(
        Request::TYPE_CHECKPOINT, Request(), config_.GetSelfInfo().id());
    checkpoint_request->set_seq(last_seq);
    checkpoint_data.SerializeToString(checkpoint_request->mutable_data());
    replica_client_->BroadCast(*checkpoint_request);
    last_seq = checkpoint_data.seq();
    sleep(1);
    continue;
  }
  return;
}

}  // namespace resdb
