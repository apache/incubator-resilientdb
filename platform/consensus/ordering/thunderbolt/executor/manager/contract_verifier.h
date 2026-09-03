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

#include "absl/status/statusor.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/committer_context.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_committer.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/contract_executor.h"

namespace resdb {
namespace contract {

class ContractVerifier : public ContractCommitter {
 public:
  ContractVerifier();
  virtual ~ContractVerifier();

  virtual bool VerifyContract(
      const std::vector<ContractExecuteInfo>& request_list,
      const std::vector<ConcurrencyController ::ModifyMap>& rws_list) = 0;

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      std::vector<ContractExecuteInfo>& request) override;

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state) override;

 protected:
  std::unique_ptr<ContractExecutor> executor_;
};

}  // namespace contract
}  // namespace resdb
