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

//#include "platform/config/resdb_config_utils.h"
#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/streaming_committer.h"
#include "proto/contract/func_params.pb.h"
#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

class ContractTransactionManager : public TransactionManager {
 public:
  ContractTransactionManager(Storage* storage);
  virtual ~ContractTransactionManager() = default;

  virtual std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override;

  bool VerifyAndExecuteRequest(const BatchUserRequest& batch_request);

  std::unique_ptr<std::vector<std::unique_ptr<google::protobuf::Message>>>
  Prepare(const BatchUserRequest& request) override;

  std::unique_ptr<BatchUserResponse> ExecutePreparedData(
      const BatchUserRequest& batch_request) override;

  bool ExecuteRequest(const BatchUserRequest& batch_request);
  void AsyncExe(std::unique_ptr<resdb::Request> request,
                const BatchUserRequest& batch_request);

 private:
  void Execute(const Request& request);

 private:
  std::unique_ptr<resdb::contract::x_manager::ContractManager> manager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> address_manager_;

  typedef std::map<int, std::pair<std::unique_ptr<ContractExecuteInfo>,
                                  std::unique_ptr<ModifyMap>>>
      DataType;
  std::map<int, DataType> data_;
  std::map<int, std::unique_ptr<resdb::Request>> req_seq_;
  std::unique_ptr<resdb::contract::x_manager::StreamingCommitter> dg_committer_;
  std::map<int, std::unique_ptr<BatchUserRequest>> batch_req_;

  std::mutex mutex_, req_mutex_;
};

}  // namespace contract
}  // namespace resdb
