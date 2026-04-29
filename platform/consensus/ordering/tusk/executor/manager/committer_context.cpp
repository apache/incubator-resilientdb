/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "service/contract/executor/manager/committer_context.h"

#include "glog/logging.h"

namespace resdb {
namespace contract {

ExecutionContext::ExecutionContext(const ContractExecuteInfo & info ) {
  info_ = std::make_unique<ContractExecuteInfo>(info);
}

const ContractExecuteInfo * ExecutionContext::GetContractExecuteInfo() const {
  return info_.get();
}

ContractExecuteInfo * ExecutionContext::GetContractExecuteInfo() {
  return info_.get();
}

void ExecutionContext::SetResult(std::unique_ptr<ExecuteResp> result) {
  result_ = std::move(result);
}

bool ExecutionContext::IsRedo() {
  return is_redo_;
}

void ExecutionContext::SetRedo(){
  is_redo_++;
}

int ExecutionContext::RedoTime() {
  return is_redo_;
}


std::unique_ptr<ExecuteResp> ExecutionContext::FetchResult() {
  return std::move(result_);
}

}  // namespace contract
}  // namespace resdb
