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

#include "platform/consensus/ordering/thunderbolt/executor/x_manager/contract_committer.h"

namespace resdb {
namespace contract {
namespace x_manager {

class ExecutionContext {
 public:
  ExecutionContext(const ContractExecuteInfo& info);
  const ContractExecuteInfo* GetContractExecuteInfo() const;
  ContractExecuteInfo* GetContractExecuteInfo();

  void SetRedo();
  bool IsRedo();
  int RedoTime();

  void SetResult(std::unique_ptr<ExecuteResp> result);
  std::unique_ptr<ExecuteResp> FetchResult();

  int64_t start_time = 0;

 private:
  int is_redo_ = 0;
  std::unique_ptr<ExecuteResp> result_;
  std::unique_ptr<ContractExecuteInfo> info_;
};

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
