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

#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/concurrency_controller.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_manager.h"
#include "proto/contract/func_params.pb.h"
#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

using Data = ::resdb::contract::Data;
using ::resdb::contract::LOAD;
using ::resdb::contract::STORE;
using ModifyMap = x_manager::ConcurrencyController::ModifyMap;

class ExecutionHelper {
 public:
  static bool DeployWithAddress(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager,
      resdb::contract::x_manager::ContractManager* manager);

  static absl::StatusOr<Contract> Deploy(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager,
      resdb::contract::x_manager::ContractManager* contract_manager);

  static std::unique_ptr<ContractExecuteInfo> GetContractInfo(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager);

  static absl::StatusOr<Contract> Deploy(
      const resdb::contract::Request& request);
  static absl::StatusOr<Account> CreateAccount(
      resdb::contract::x_manager::AddressManager* address_manager);
  static absl::StatusOr<Account> CreateAccountWithAddress(
      const std::string& address,
      resdb::contract::x_manager::AddressManager* address_manager);

  static bool DeployWithAddress(
      const resdb::contract::Request& request,
      resdb::contract::x_manager::AddressManager* address_manager);
  static bool CreateAccount(
      const std::string& address,
      resdb::contract::x_manager::AddressManager* address_manager);

  static std::unique_ptr<ModifyMap> GetRWSList(const ResultInfo& request);
};

}  // namespace contract
}  // namespace resdb
