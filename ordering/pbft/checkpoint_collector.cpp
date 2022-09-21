#include "ordering/pbft/checkpoint_collector.h"

#include <glog/logging.h>

namespace resdb {

CheckPointCollector::CheckPointCollector(const ResDBConfig& config,
                                         CheckPointInfo* checkpoint_info)
    : config_(config), checkpoint_info_(checkpoint_info) {}

void CheckPointCollector::MayNewCollector(
    std::map<uint64_t, std::unique_ptr<TransactionCollector>>* collector,
    uint64_t seq, std::shared_mutex* collector_mutex) {
  {
    std::shared_lock<std::shared_mutex> lock(*collector_mutex);
    if (collector->find(seq) != collector->end()) {
      return;
    }
  }

  std::unique_lock<std::shared_mutex> lock(*collector_mutex);
  if (collector->find(seq) == collector->end()) {
    (*collector)[seq] = std::make_unique<TransactionCollector>(seq, nullptr);
  }
}

bool CheckPointCollector::MayConsensusChangeStatus(
    int type, int received_count, std::atomic<TransactionStatue>* status) {
  switch (type) {
    case Request::TYPE_CHECKPOINT:
      // if receive 2f+1 checkpoint, update stable checkpoint
      if (*status == TransactionStatue::None &&
          config_.GetMinDataReceiveNum() <= received_count) {
        *status = TransactionStatue::READY_EXECUTE;
        return true;
      }
      break;
  }
  return false;
}

// TODO merge code.
CollectorResultCode CheckPointCollector::AddCheckPointMsg(
    const SignatureInfo& signature, std::unique_ptr<Request> request) {
  if (request == nullptr) {
    return CollectorResultCode::INVALID;
  }

  int type = request->type();
  uint64_t seq = request->seq();

  MayNewCollector(&checkpoint_message_, seq, &checkpoint_mutex_);

  LOG(INFO) << "add checkpoint data: view[" << request->current_view()
            << "] seq [" << request->seq() << "]"
            << " type:" << request->type();
  bool success = false;
  std::shared_lock<std::shared_mutex> lock(checkpoint_mutex_);
  int ret = checkpoint_message_[seq]->AddRequest(
      std::move(request), signature, false,
      [&](const Request& request, int received_count,
          const TransactionCollector::CollectorDataType* data_set,
          std::atomic<TransactionStatue>* status) {
        if (MayConsensusChangeStatus(type, received_count, status)) {
          success = true;

          std::vector<CheckPointData> datas;
          for (const auto& it : *data_set) {
            if (it->request == nullptr) {
              LOG(ERROR) << "request is empty, data invalid";
              return;
            }
            CheckPointData data;
            if (!data.ParseFromString(request.data())) {
              return;
            }
            datas.push_back(data);
          }
          checkpoint_info_->UpdateStableCheckPoint(datas);
        }
      });
  if (ret == 0) {
    return CollectorResultCode::OK;
  }
  return CollectorResultCode::INVALID;
}

}  // namespace resdb
