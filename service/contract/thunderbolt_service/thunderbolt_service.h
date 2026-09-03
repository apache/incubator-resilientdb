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

#include "platform/consensus/ordering/thunderbolt/framework/consensus.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/address_manager.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_manager.h"
#include "proto/contract/func_params.pb.h"
#include "proto/contract/rpc.pb.h"

namespace resdb {
namespace contract {

class Thunderbolt : public thunderbolt::ThunderboltConsensus {
 public:
  Thunderbolt(const ResDBConfig& config,
       std::unique_ptr<resdb::TransactionManager> executor);

  int ProcessRequest(resdb::Request* request);

 private:
  /*
    ContractExecuteInfo GetContractInfo(const resdb::contract::Request
   &request);
    //ConcurrencyController :: ModifyMap GetRWSList(const
   resdb::contract::ResultInfo &request);

   absl::StatusOr<Contract> Deploy(const resdb::contract::Request& request);
   absl::StatusOr<Account> CreateAccount();
   absl::StatusOr<Account> CreateAccountWithAddress(const std::string& address)
   ;

   bool DeployWithAddress(const resdb::contract::Request& request);
   bool CreateAccount(const std::string& address) ;
   */

 private:
  std::unique_ptr<resdb::contract::x_manager::ContractManager> manager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> address_manager_;

  std::unique_ptr<resdb::contract::x_manager::ContractManager> emanager_;
  std::unique_ptr<resdb::contract::x_manager::AddressManager> eaddress_manager_;
  std::mutex mutex_;
  int worker_num_;
};

}  // namespace contract
}  // namespace resdb
