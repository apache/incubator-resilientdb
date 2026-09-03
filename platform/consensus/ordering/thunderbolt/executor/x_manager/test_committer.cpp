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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/test_committer.h"

#include "glog/logging.h"
//#include "eEVM/processor.h"

namespace resdb {
namespace contract {

TestCommitter::TestCommitter(DataStorage* storage, GlobalState* global_state)
    : gs_(global_state) {
  controller_ = std::make_unique<TestController>(storage);
  executor_ = std::make_unique<ContractExecutor>();
}

TestCommitter::~TestCommitter() {}

std::vector<std::unique_ptr<ExecuteResp>> TestCommitter::ExecContract(
    const std::vector<ContractExecuteInfo>& requests) {
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  for (const auto& request : requests) {
    std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
    // auto start_time = GetCurrentTime();
    auto ret = ExecContract(request.caller_address, request.contract_address,
                            request.func_addr, request.func_params, gs_);
    resp->state = ret.status();
    if (ret.ok()) {
      resp->ret = 0;
      resp->result = *ret;
    } else {
      LOG(ERROR) << "exec fail";
      resp->ret = -1;
    }
    resp->contract_address = request.contract_address;
    resp->commit_id = request.commit_id;

    resp_list.push_back(std::move(resp));
  }
  return resp_list;
}

absl::StatusOr<std::string> TestCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace contract
}  // namespace resdb
