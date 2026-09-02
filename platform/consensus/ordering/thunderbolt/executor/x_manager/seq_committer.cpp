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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/seq_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/exception.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/executor_state.h"

//#define Debug

namespace resdb {
namespace contract {
namespace x_manager {

SeqCommitter::SeqCommitter(DataStorage* storage, GlobalState* global_state,
                           int window_size, int worker_num)
    : gs_(global_state), worker_num_(worker_num), window_size_(window_size) {
  executor_ = std::make_unique<ContractExecutor>();
}

SeqCommitter::~SeqCommitter() {
  // LOG(ERROR)<<"desp";
  is_stop_ = true;
}

void SeqCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  return;
}

std::vector<std::unique_ptr<ExecuteResp>> SeqCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  for (auto& request : requests) {
    auto ret = ExecContract(request.caller_address, request.contract_address,
                            request.func_addr, request.func_params, gs_);
    std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
    resp->contract_address = request.contract_address;
    resp->commit_id = request.commit_id;
    resp->user_id = request.user_id;
    resp->ret = 0;
    resp->result = *ret;
    resp_list.push_back(std::move(resp));
  }
  return resp_list;
}

absl::StatusOr<std::string> SeqCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
