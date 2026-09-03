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
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/x_verifier.h"

#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "platform/consensus/ordering/thunderbolt/executor/x_manager/local_state.h"

namespace resdb {
namespace contract {
namespace x_manager {

XVerifier::XVerifier(DataStorage* storage, GlobalState* global_state,
                     int worker_num)
    : storage_(storage), gs_(global_state), worker_num_(worker_num) {
  controller_ = std::make_unique<VController>(storage);
  is_stop_ = false;

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        auto request = request_queue_.Pop();
        if (request == nullptr) {
          continue;
        }

        LocalState local_state(controller_.get());
        local_state.Set(
            gs_->GetAccount(
                request->GetContractExecuteInfo()->contract_address),
            request->GetContractExecuteInfo()->commit_id);

        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        auto ret = ExecContract(
            request->GetContractExecuteInfo()->caller_address,
            request->GetContractExecuteInfo()->contract_address,
            request->GetContractExecuteInfo()->func_addr,
            request->GetContractExecuteInfo()->func_params, &local_state);
        resp->state = ret.status();
        resp->contract_address =
            request->GetContractExecuteInfo()->contract_address;
        resp->commit_id = request->GetContractExecuteInfo()->commit_id;
        resp->user_id = request->GetContractExecuteInfo()->user_id;
        if (ret.ok()) {
          resp->ret = 0;
          resp->result = *ret;
          if (request->IsRedo()) {
            resp->retry_time++;
          }
          local_state.Flesh(request->GetContractExecuteInfo()->contract_address,
                            request->GetContractExecuteInfo()->commit_id);
        } else {
          resp->ret = -1;
        }
        resp_queue_.Push(std::move(resp));
      }
    }));
  }
}

XVerifier::~XVerifier() {
  is_stop_ = true;
  for (int i = 0; i < worker_num_; ++i) {
    workers_[i].join();
  }
}

bool XVerifier::VerifyContract(
    const std::vector<ContractExecuteInfo>& request_list,
    const std::vector<ConcurrencyController ::ModifyMap>& rws_list) {
  std::vector<int> d;
  std::map<int, std::vector<int> > g;
  std::map<Address, int> id;

  for (int i = 0; i < rws_list.size(); ++i) {
    d.push_back(0);
    for (auto it : rws_list[i]) {
      const Address& address = it.first;
      // LOG(ERROR)<<"i ="<<i<<" address:"<<address;
      if (id.find(address) == id.end() || id[address] == i) {
        id[address] = i;
        continue;
      }
      g[id[address]].push_back(i);
      // LOG(ERROR)<<"add edge:"<<id[address]<<" to "<<i;
      id[address] = i;
      d[i]++;
    }
  }

  std::queue<int> q;
  for (int i = 0; i < request_list.size(); ++i) {
    if (d[i] == 0) {
      auto context = std::make_unique<ExecutionContext>(request_list[i]);
      context->GetContractExecuteInfo()->commit_id = i;
      request_queue_.Push(std::move(context));
      // LOG(ERROR)<<"add:"<<i;
    }
  }

  int process_num = request_list.size();
  while (process_num > 0) {
    auto resp = resp_queue_.Pop();
    if (resp == nullptr) {
      continue;
    }
    process_num--;
    int resp_commit_id = resp->commit_id;
    bool ret = controller_->Commit(resp_commit_id);
    if (!ret) {
      return false;
    }
    // LOG(ERROR)<<"verify:"<<resp_commit_id;
    auto* rws = controller_->GetChangeList(resp_commit_id);
    if (!RWSEqual(*rws, rws_list[resp_commit_id])) {
      LOG(ERROR) << "rws not equal";
      return false;
    }
    for (int n_id : g[resp_commit_id]) {
      // LOG(ERROR)<<"next id:"<<n_id;
      d[n_id]--;
      if (d[n_id] == 0) {
        auto context = std::make_unique<ExecutionContext>(request_list[n_id]);
        context->GetContractExecuteInfo()->commit_id = n_id;
        request_queue_.Push(std::move(context));
        // LOG(ERROR)<<"add next:"<<n_id;
      }
    }
  }
  // LOG(ERROR)<<"verified";
  return true;
}

bool XVerifier::RWSEqual(const ConcurrencyController ::ModifyMap& a,
                         const ConcurrencyController ::ModifyMap& b) {
  // LOG(ERROR)<<"size:"<<a.size()<<" "<<b.size();
  if (a.size() != b.size()) {
    return false;
  }
  for (auto it1 = a.begin(), it2 = b.begin(); it1 != a.end() && it2 != b.end();
       it1++, it2++) {
    if (it1->first != it2->first) {
      LOG(ERROR) << "address not equal:" << it1->first << " " << it2->first;
      return false;
    }

    /*
        if(it1->second.size() != it2->second.size()){
          LOG(ERROR)<<"item size not equal:"<<it1->second.size()<<"
       "<<it2->second.size()<<" address:"<<it1->first; return false;
        }
        */

    int last2 = -1, last1 = -1;
    for (int i = it2->second.size() - 1; i >= 0; i--) {
      if (it2->second[i].state == STORE) {
        last2 = i;
        break;
      }
    }

    for (int i = it1->second.size() - 1; i >= 0; i--) {
      if (it1->second[i].state == STORE) {
        last1 = i;
        break;
      }
    }

    if (last2 == -1 && last1 == -1) {
      continue;
    }
    if (last2 >= 0 && last1 >= 0) {
      if (it1->second[last1].data != it2->second[last2].data) {
        LOG(ERROR) << "data not equal:" << it1->first
                   << " data:" << it1->second[last1].data << " "
                   << it2->second[last2].data;
        return false;
      }
      continue;
    }
    LOG(ERROR) << "write set not equal:" << it1->first;
  }
  return true;
}

}  // namespace x_manager
}  // namespace contract
}  // namespace resdb
