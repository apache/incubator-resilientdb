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

#include "platform/consensus/execution/geo_transaction_executor.h"

#include <glog/logging.h>

#include "platform/consensus/ordering/common/transaction_utils.h"

namespace resdb {

GeoTransactionExecutor::GeoTransactionExecutor(
    const ResDBConfig& config, std::unique_ptr<SystemInfo> system_info,
    std::unique_ptr<ReplicaCommunicator> replica_communicator,
    std::unique_ptr<TransactionManager> local_transaction_manager)
    : TransactionManager(false, false),
      config_(config),
      system_info_(std::move(system_info)),
      replica_communicator_(std::move(replica_communicator)),
      local_transaction_manager_(std::move(local_transaction_manager)),
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

  int self_send = replica_communicator_->SendBatchMessage(
      batch_geo_request, config_.GetSelfInfo());
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
        int ret =
            replica_communicator_->SendBatchMessage(batch_geo_request, replica);
        if (ret >= 0) {
          num_request_sent++;
        }
      }
    }
    // LOG(ERROR) << "local executor send out geo message";
  }
}

std::unique_ptr<BatchUserResponse> GeoTransactionExecutor::ExecuteBatch(
    const BatchUserRequest& request) {
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
