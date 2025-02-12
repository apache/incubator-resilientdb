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

#include "executor/common/transaction_manager.h"
#include "executor/contract/manager/address_manager.h"
#include "executor/contract/manager/contract_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "proto/contract/func_params.pb.h"
#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

class ContractTransactionManager : public TransactionManager {
 public:
  ContractTransactionManager(void);
  virtual ~ContractTransactionManager() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 private:
  absl::StatusOr<Account> CreateAccount();
  absl::StatusOr<Contract> Deploy(const Request& request);
  absl::StatusOr<std::string> Execute(const Request& request);
  absl::Status AddAddress(const Request& request);

 private:
  std::unique_ptr<ContractManager> contract_manager_;
  std::unique_ptr<AddressManager> address_manager_;
};

}  // namespace contract
}  // namespace resdb
