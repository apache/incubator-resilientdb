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

#include "execution/geo_transaction_executor.h"

#include <glog/logging.h>

#include "ordering/pbft/transaction_utils.h"

namespace resdb {

GeoTransactionExecutor::GeoTransactionExecutor(
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    std::unique_ptr<ResDBReplicaClient> replica_client,
    std::unique_ptr<TransactionExecutorImpl> geo_executor_impl)
    : TransactionExecutorImpl(false, false),
      config_(config),
      system_info_(std::move(system_info)),
      replica_client_(std::move(replica_client)),
      geo_executor_impl_(std::move(geo_executor_impl)),
      is_stop_(false) {
  geo_thread_ = std::thread(&GeoTransactionExecutor::SendGeoMessages, this);
}

GeoTransactionExecutor::~GeoTransactionExecutor() {
  is_stop_ = true;
  if (geo_thread_.joinable()) {
    geo_thread_.join();
  }
}

bool GeoTransactionExecutor::IsStop() { return is_stop_; }

void GeoTransactionExecutor::SendGeoMessages() {
  std::vector<std::unique_ptr<Request>> messages;
  while (!IsStop()) {
    auto message = queue_.Pop(100);
    if (message != nullptr) {
      messages.push_back(std::move(message));
      while (!IsStop() && messages.size() < batch_size_) {
        auto message = queue_.Pop(0);
        if (message == nullptr) {
          break;
        }
        messages.push_back(std::move(message));
        if (messages.size() >= batch_size_) {
          break;
        }
      }
    }
    if (messages.size() > 0) {
      SendBatchGeoMessage(messages);
      messages.clear();
    }
  }
  return;
}

void GeoTransactionExecutor::SendBatchGeoMessage(
    const std::vector<std::unique_ptr<Request>>& batch_geo_request) {
  ResConfigData config_data = config_.GetConfigData();

  int self_send = replica_client_->SendBatchMessage(batch_geo_request,
                                                    config_.GetSelfInfo());
  if (self_send < 0) {
    LOG(ERROR) << "send batch_geo_request to self FAIL!";
  }
  // Only for primary node: send out GEO_REQUEST to other regions.
  if (config_.GetSelfInfo().id() == system_info_->GetPrimaryId()) {
    for (const auto& region : config_data.region()) {
      if (region.region_id() == config_data.self_region_id()) {
        continue;
      }
      // maximum number of faulty replicas in this region
      int max_faulty = (region.replica_info_size() - 1) / 3;
      int num_request_sent = 0;
      for (const auto& replica : region.replica_info()) {
        // send to f + 1 replicas in the region
        if (num_request_sent > max_faulty) {
          break;
        }
        int ret = replica_client_->SendBatchMessage(batch_geo_request, replica);
        if (ret >= 0) {
          num_request_sent++;
        }
      }
    }
    // LOG(ERROR) << "local executor send out geo message";
  }
}

std::unique_ptr<BatchClientResponse> GeoTransactionExecutor::ExecuteBatch(
    const BatchClientRequest& request) {
  std::unique_ptr<Request> geo_request = resdb::NewRequest(
      Request::TYPE_GEO_REQUEST, Request(), config_.GetSelfInfo().id(),
      config_.GetConfigData().self_region_id());

  geo_request->set_seq(request.seq());
  geo_request->set_proxy_id(request.proxy_id());
  geo_request->set_hash(SignatureVerifier::CalculateHash(
      geo_request->data() + std::to_string(request.seq()) +
      std::to_string(config_.GetConfigData().self_region_id())));

  request.SerializeToString(geo_request->mutable_data());

  queue_.Push(std::move(geo_request));
  return nullptr;
}

}  // namespace resdb
