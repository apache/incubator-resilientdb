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

#include "service/contract/executor/manager/x_verifier.h"

#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/manager/local_state.h"

namespace resdb {
namespace contract {

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
    if (it1->second.size() != it2->second.size()) {
      LOG(ERROR) << "item size not equal:" << it1->second.size() << " "
                 << it2->second.size();
      return false;
    }
    for (int i = 0; i < it1->second.size(); i++) {
      if (it1->second[i] != it2->second[i]) {
        LOG(ERROR) << "data not equal";
        return false;
      }
    }
  }
  return true;
}

}  // namespace contract
}  // namespace resdb
