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
#include "service/contract/thunderbolt_service/contract_transaction_manager.h"

#include <glog/logging.h>

#include "service/contract/thunderbolt_service/execution_helper.h"

namespace resdb {
namespace contract {

ContractTransactionManager::ContractTransactionManager(Storage* storage)
    : TransactionManager(false, true) {
  (void)storage;
  manager_ = std::make_unique<x_manager::ContractManager>(
      std::make_unique<DataStorage>(), 16,
      x_manager::ContractManager::Options::FX);
  address_manager_ = std::make_unique<x_manager::AddressManager>();
}

void ContractTransactionManager::SetAsyncCallback(
    std::function<void(const BatchUserRequest, std::unique_ptr<resdb::Request>,
                       std::unique_ptr<BatchUserResponse>)>
        callback) {
  async_callback_ = std::move(callback);
}

std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
ContractTransactionManager::Prepare(const BatchUserRequest& request) {
  (void)request;
  return nullptr;
}

std::unique_ptr<BatchUserResponse>
ContractTransactionManager::ExecutePreparedData(
    const BatchUserRequest& request) {
  return ExecuteBatch(request);
}

bool ContractTransactionManager::ExecuteContractRequest(
    const Request& request, Response* response) {
  const Address caller = x_manager::AddressManager::HexToAddress(
      request.caller_address());
  if (!address_manager_->Exist(caller)) {
    response->set_ret(-1);
    return false;
  }

  auto result = manager_->ExecContract(
      caller,
      x_manager::AddressManager::HexToAddress(request.contract_address()),
      request.func_params());
  if (!result.ok()) {
    response->set_ret(-1);
    return false;
  }
  response->set_res(*result);
  response->set_ret(0);
  return true;
}

bool ContractTransactionManager::ExecuteRequest(
    const BatchUserRequest& batch_request) {
  auto response_batch = std::make_unique<BatchUserResponse>();
  for (int i = 0; i < batch_request.user_requests_size(); ++i) {
    const auto& sub_request = batch_request.user_requests(i);
    Request request;
    if (!request.ParseFromString(sub_request.request().data())) {
      return false;
    }

    Response response;
    bool ok = true;
    switch (request.cmd()) {
      case Request::CREATE_ACCOUNT: {
        auto account = ExecutionHelper::CreateAccount(address_manager_.get());
        if (!account.ok()) {
          ok = false;
          response.set_ret(-1);
        } else {
          response.mutable_account()->CopyFrom(*account);
          response.set_ret(0);
        }
        break;
      }
      case Request::DEPLOY: {
        auto contract = ExecutionHelper::Deploy(
            request, address_manager_.get(), manager_.get());
        if (!contract.ok()) {
          ok = false;
          response.set_ret(-1);
        } else {
          response.mutable_contract()->CopyFrom(*contract);
          response.set_ret(0);
        }
        break;
      }
      case Request::EXECUTE:
        ok = ExecuteContractRequest(request, &response);
        break;
      default:
        response.set_ret(0);
        break;
    }

    response_batch->add_response(response.SerializeAsString());
    if (!ok) {
      LOG(ERROR) << "failed to execute contract request";
    }
  }

  if (async_callback_) {
    auto request = std::make_unique<resdb::Request>();
    request->set_data(batch_request.SerializeAsString());
    async_callback_(batch_request, std::move(request),
                    std::move(response_batch));
    return true;
  }
  return true;
}

std::unique_ptr<BatchUserResponse> ContractTransactionManager::ExecuteBatch(
    const BatchUserRequest& request) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto response = std::make_unique<BatchUserResponse>();
  if (!ExecuteRequest(request)) {
    return nullptr;
  }
  response->set_seq(request.seq());
  return response;
}

}  // namespace contract
}  // namespace resdb
