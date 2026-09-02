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
#include "platform/consensus/ordering/thunderbolt/executor/manager/streaming_single_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"

namespace resdb {
namespace contract {
namespace streaming {

StreamingSingleCommitter::StreamingSingleCommitter(
    DataStorage* storage, GlobalState* global_state, int window_size,
    std::function<void(std::unique_ptr<ExecuteResp>)> call_back, int worker_num)
    : storage_(storage), gs_(global_state), call_back_(call_back) {
  executor_ = std::make_unique<ContractExecutor>();

  id_ = 1;
}

void StreamingSingleCommitter::SetExecuteCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  call_back_ = std::move(func);
}

StreamingSingleCommitter::~StreamingSingleCommitter() {}

void StreamingSingleCommitter::Execute(const ContractExecuteInfo& request) {
  std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
  auto ret = ExecContract(request.caller_address, request.contract_address,
                          request.func_addr, request.func_params, gs_);
  resp->state = ret.status();
  resp->contract_address = request.contract_address;
  resp->commit_id = request.commit_id;
  resp->user_id = request.user_id;
  if (ret.ok()) {
    resp->ret = 0;
    resp->result = *ret;
    // LOG(ERROR)<<"commit :"<<resp->commit_id;
  } else {
    LOG(ERROR) << "commit :" << resp->commit_id << " fail";
    resp->ret = -1;
    assert(resp->ret >= 0);
  }
  if (call_back_) {
    call_back_(std::move(resp));
  }
}

void StreamingSingleCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  for (auto& request : requests) {
    request.commit_id = id_++;
    Execute(request);
  }

  return;
}

absl::StatusOr<std::string> StreamingSingleCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace streaming
}  // namespace contract
}  // namespace resdb
