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
#include "platform/consensus/ordering/thunderbolt/executor/manager/two_phase_committer.h"

#include "eEVM/processor.h"
#include "glog/logging.h"
#include "platform/consensus/ordering/thunderbolt/executor/manager/local_state.h"

namespace resdb {
namespace contract {

TwoPhaseCommitter::TwoPhaseCommitter(DataStorage* storage,
                                     GlobalState* global_state, int worker_num)
    : gs_(global_state), worker_num_(worker_num) {
  controller_ = std::make_unique<TwoPhaseController>(storage);
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

        // LOG(ERROR)<<"worker:"<<i<<" process
        // contract:"<<request->contract_address<<" commit
        // id:"<<request->commit_id;
        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        // auto start_time = GetCurrentTime();
        auto ret = ExecContract(request->caller_address,
                                request->contract_address, request->func_addr,
                                request->func_params, &local_state);
        resp->state = ret.status();
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
          local_state.Flesh(request->contract_address, request->commit_id);
        } else {
          LOG(ERROR) << "exec fail";
          resp->ret = -1;
        }
        // LOG(ERROR)<<"execute:"<<GetCurrentTime() - start_time;
        resp->contract_address = request->contract_address;
        resp->commit_id = request->commit_id;
        resp->user_id = request->user_id;
        resp_queue_.Push(std::move(resp));
      }
    }));
  }
}

TwoPhaseCommitter::~TwoPhaseCommitter() {
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

std::vector<std::unique_ptr<ExecuteResp>> TwoPhaseCommitter::ExecContract(
    std::vector<ContractExecuteInfo>& requests) {
  // LOG(ERROR)<<"executor contract size:"<<requests.size();
  std::set<int64_t> fail_list;
  std::map<int64_t, std::unique_ptr<ExecuteResp>> responses;

  int do_time = 0;
  // auto start_time = GetCurrentTime();
  do {
    controller_->Clear();

    // process
    int process_num = 0;
    int receive_num = 0;
    for (auto request : requests) {
      if (fail_list.empty()) {
        // first try
        // LOG(ERROR)<<"try commit id:"<<request.commit_id<<"
        // contract:"<<request.contract_address;
        request_queue_.Push(std::make_unique<ContractExecuteInfo>(request));
        process_num++;
      } else {
        if (fail_list.find(request.commit_id) != fail_list.end()) {
          // re-do
          // LOG(ERROR)<<"re-do commit id:"<<request.commit_id<<"
          // contract:"<<request.contract_address;
          request_queue_.Push(std::make_unique<ContractExecuteInfo>(request));
          process_num++;
        }
      }
    }

    std::map<int64_t, std::unique_ptr<ExecuteResp>> tmp_responses;

    while (process_num != receive_num) {
      auto resp = resp_queue_.Pop();
      if (resp == nullptr) {
        continue;
      }
      tmp_responses[resp->commit_id] = std::move(resp);
      receive_num++;
    }

    // commit
    fail_list.clear();
    for (auto& resp_it : tmp_responses) {
      auto& resp = resp_it.second;
      if (resp->ret == 0) {
        // gs_->Flesh(resp->contract_address, resp->commit_id);
        if (!controller_->Commit(resp->commit_id)) {
          // LOG(ERROR)<<"commit fail, contract
          // address:"<<resp->contract_address<<" commit id:"<<resp->commit_id;
          fail_list.insert(resp->commit_id);
        } else {
          // LOG(ERROR)<<"commit done, contract
          // address:"<<resp->contract_address<<" commit id:"<<resp->commit_id;
        }
      }
      resp->retry_time = do_time;
      responses[resp_it.first] = std::move(resp);
    }
    do_time++;
  } while (!fail_list.empty());

  std::vector<std::unique_ptr<ExecuteResp>> resp_list;
  for (auto& resp_it : responses) {
    resp_list.push_back(std::move(resp_it.second));
  }
  // if(do_time>1)
  // LOG(ERROR)<<"commit time:"<<do_time;

  return resp_list;
}

absl::StatusOr<std::string> TwoPhaseCommitter::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const std::string& func_addr, const Params& func_param, EVMState* state) {
  return executor_->ExecContract(caller_address, contract_address, func_addr,
                                 func_param, state);
}

}  // namespace contract
}  // namespace resdb
