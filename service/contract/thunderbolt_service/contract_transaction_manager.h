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
#pragma once

#include <functional>
#include <memory>

#include "executor/common/transaction_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_manager.h"
#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

class ContractTransactionManager : public TransactionManager {
 public:
  explicit ContractTransactionManager(Storage* storage);
  ~ContractTransactionManager() override = default;

  std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override;
  std::unique_ptr<BatchUserResponse> ExecutePreparedData(
      const BatchUserRequest& request) override;

  std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
  Prepare(const BatchUserRequest& request) override;

  void SetAsyncCallback(
      std::function<void(const BatchUserRequest, std::unique_ptr<resdb::Request>,
                         std::unique_ptr<BatchUserResponse>)> callback) override;

 private:
  bool ExecuteRequest(const BatchUserRequest& request);
  bool ExecuteContractRequest(const Request& request, Response* response);

  std::unique_ptr<x_manager::ContractManager> manager_;
  std::unique_ptr<x_manager::AddressManager> address_manager_;
  std::function<void(const BatchUserRequest, std::unique_ptr<resdb::Request>,
                     std::unique_ptr<BatchUserResponse>)>
      async_callback_;
  std::mutex mutex_;
};

}  // namespace contract
}  // namespace resdb
