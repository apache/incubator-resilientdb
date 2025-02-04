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

#include "service/contract/executor/x_manager/seq_committer.h"

#include <future>
#include <queue>

#include "common/utils/utils.h"
#include "eEVM/exception.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/executor_state.h"

//#define Debug

namespace resdb {
namespace contract {
namespace x_manager {

SeqCommitter::SeqCommitter(DataStorage* storage, GlobalState* global_state,
                           int window_size, int worker_num)
    : gs_(global_state) {
  executor_ = std::make_unique<ContractExecutor>();
  controller_ = std::make_unique<SeqController>(storage, window_size);
}

SeqCommitter::~SeqCommitter() {
  // LOG(ERROR)<<"desp";
}

void SeqCommitter::AsyncExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  return;
}

std::unique_ptr<ExecuteResp> SeqCommitter::Execute(
    const ContractExecuteInfo* request) {
  ExecutorState executor_state(controller_.get(), request->commit_id);
  executor_state.Set(gs_->GetAccount(request->contract_address),
                     request->commit_id, 0);

  std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
  auto ret =
      ExecContract(request->caller_address, request->contract_address,
                   request->func_addr, request->func_params, &executor_state);

  resp->state = ret.status();
  resp->contract_address = request->contract_address;
  resp->commit_id = request->commit_id;
  resp->user_id = request->user_id;
  resp->rws = controller_->GetChangeList(resp->commit_id);
  // LOG(ERROR)<<"done :"<<resp->commit_id<<" wrs size:"<<resp->rws.size()<<"
  // ok?"<<ret.ok()<<" state:"<<ret.status();
  assert(resp->rws.size() > 0);
  if (ret.ok()) {
    resp->ret = 0;
    resp->result = *ret;
  } else {
    LOG(ERROR) << "commit :" << resp->commit_id << " fail";
    resp->ret = -1;
  }
  return resp;
}

std::vector<std::unique_ptr<ExecuteResp>> SeqCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  controller_->Clear();
  int id = 1;
  for (auto& request : requests) {
    request.commit_id = id++;
    auto resp = Execute(&request);
    assert(resp->ret == 0);
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
