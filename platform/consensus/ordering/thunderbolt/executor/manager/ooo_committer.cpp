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

#include "service/contract/executor/manager/ooo_committer.h"

#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/manager/local_state.h"

namespace resdb {
namespace contract {

OOOCommitter::OOOCommitter(DataStorage* storage, GlobalState* global_state,
                           int worker_num)
    : gs_(global_state), worker_num_(worker_num) {
  controller_ = std::make_unique<OOOController>(storage);
  executor_ = std::make_unique<ContractExecutor>();
  is_stop_ = false;

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request = request_queue_.Pop();
        if (request == nullptr) {
          continue;
        }
        LocalState local_state(controller_.get());
        local_state.Set(gs_->GetAccount(request->contract_address),
                        request->commit_id);

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        auto ret = ExecContract(request->caller_address,
                                request->contract_address, request->func_addr,
                                request->func_params, &local_state);
        resp->state = ret.status();
        resp->contract_address = request->contract_address;
        resp->commit_id = request->commit_id;
        resp->user_id = request->user_id;
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
        } else {
          resp->ret = -1;
        }
        resp_queue_.Push(std::move(resp));
      }
    }));
  }
}

OOOCommitter::~OOOCommitter() {
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

std::vector<std::unique_ptr<ExecuteResp>> OOOCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  int process_num = requests.size();
  std::vector<std::unique_ptr<ExecuteResp>> resp_list;

  std::set<int64_t> commits;
  resp_list.resize(process_num);

  for (auto& request : requests) {
    request_queue_.Push(std::make_unique<ContractExecuteInfo>(request));
  }

  while (process_num) {
    auto resp = resp_queue_.Pop();
    if (resp == nullptr) {
      continue;
    }
    int64_t resp_commit_id = resp->commit_id;
    resp_list[resp->commit_id - 1] = std::move(resp);
    process_num--;
  }

  return resp_list;
}

absl::StatusOr<std::string> OOOCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace contract
}  // namespace resdb
