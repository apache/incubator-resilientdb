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
#include "platform/consensus/ordering/thunderbolt/executor/manager/committer_context.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

ExecutionContext::ExecutionContext(const ContractExecuteInfo& info) {
  info_ = std::make_unique<ContractExecuteInfo>(info);
}

const ContractExecuteInfo* ExecutionContext::GetContractExecuteInfo() const {
  return info_.get();
}

ContractExecuteInfo* ExecutionContext::GetContractExecuteInfo() {
  return info_.get();
}

void ExecutionContext::SetResult(std::unique_ptr<ExecuteResp> result) {
  result_ = std::move(result);
}

bool ExecutionContext::IsRedo() { return is_redo_; }

void ExecutionContext::SetRedo() { is_redo_++; }

int ExecutionContext::RedoTime() { return is_redo_; }

std::unique_ptr<ExecuteResp> ExecutionContext::FetchResult() {
  return std::move(result_);
}

}  // namespace contract
}  // namespace resdb
