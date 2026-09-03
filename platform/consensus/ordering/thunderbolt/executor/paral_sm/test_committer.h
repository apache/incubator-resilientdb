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

#include <thread>

#include "absl/status/statusor.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_committer.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/contract_executor.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/global_state.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/test_controller.h"
#include "proto/contract/func_params.pb.h"

namespace resdb {
namespace contract {

class TestCommitter : public ContractCommitter {
 public:
  TestCommitter(DataStorage* storage, GlobalState* global_state);

  virtual ~TestCommitter();

  std::vector<std::unique_ptr<ExecuteResp>> ExecContract(
      const std::vector<ContractExecuteInfo>& request);

  absl::StatusOr<std::string> ExecContract(const Address& caller_address,
                                           const Address& contract_address,
                                           const std::string& func_addr,
                                           const Params& func_param,
                                           EVMState* state);

 private:
  std::unique_ptr<TestController> controller_;
  GlobalState* gs_;
  std::unique_ptr<ContractExecutor> executor_;
};

}  // namespace contract
}  // namespace resdb
