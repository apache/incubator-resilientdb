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

#include "service/contract/executor/x_manager/x_verifier.h"

#include <queue>

#include "common/utils/utils.h"
#include "eEVM/processor.h"
#include "glog/logging.h"
#include "service/contract/executor/x_manager/local_state.h"

namespace resdb {
namespace contract {
namespace x_manager {

XVerifier::XVerifier(DataStorage* storage, GlobalState* global_state,
                     int worker_num)
    : storage_(storage), gs_(global_state), worker_num_(worker_num) {
  // LOG(ERROR)<<" verifier worker num:"<<worker_num_;
  controller_ = std::make_unique<VController>(storage);
  is_stop_ = false;

  for (int i = 0; i < worker_num_; ++i) {
    workers_.push_back(std::thread([&]() {
      while (!is_stop_) {
        /*
          auto request = request_queue_.Pop();
          if (request == nullptr) {
            continue;
          }
          */
        auto x = x_request_queue_.Pop();
        if (x == nullptr) {
          continue;
        }

        ContractExecuteInfo* contract_info = request_[*x].get();
        assert(contract_info != nullptr);

        LocalState local_state(controller_.get());
        local_state.Set(gs_->GetAccount(contract_info->contract_address),
                        contract_info->commit_id);

        // LOG(ERROR)<<"execute commit id:"<<contract_info->commit_id;
        std::unique_ptr<ExecuteResp> resp = std::make_unique<ExecuteResp>();
        resp->commit_id = *x;

        auto ret = ExecContract(
            contract_info->caller_address, contract_info->contract_address,
            contract_info->func_addr, contract_info->func_params, &local_state);
        // resp->state = ret.status();
        // resp->contract_address =
        // request->GetContractExecuteInfo()->contract_address; resp->commit_id =
        // request->GetContractExecuteInfo()->commit_id; LOG(ERROR)<<"commit
        // id:"<<resp->commit_id<<" execude done"; resp->user_id =
        // request->GetContractExecuteInfo()->user_id;
        if (ret.ok()) {
          resp->ret = 0;
          // resp->result = *ret;
          // if(request->IsRedo()){
          // resp->retry_time++;
          //}
          local_state.Flesh(contract_info->contract_address,
                            contract_info->commit_id);
        } else {
          resp->ret = -1;
        }
        // LOG(ERROR)<<"push commit id:"<<resp->commit_id<<" execude done";
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

bool XVerifier::VerifyContract(std::map<int, VerifyInfoType>& ranklist) {
  // LOG(ERROR)<<"verify:";
  std::vector<int> d;
  d.resize(ranklist.size());
  std::map<int, std::vector<int> > g;
  std::map<Address, int> id;

  std::set<int> done;
  for (auto& it : ranklist) {
    const ModifyMap* rws = it.second.second.get();
    int i = it.first;
    assert(i < d.size());
    d[i] = 0;
    request_[i] = std::move(it.second.first);
    request_[i]->commit_id = i;

    for (auto rws_it : *rws) {
      const Address& address = rws_it.first;
      // LOG(ERROR)<<"index i:"<<i<<" address:"<<address;
      if (id.find(address) == id.end() || id[address] == i) {
        id[address] = i;
        continue;
      }
      // there is a transaction before that contains the same address.
      g[id[address]].push_back(i);
      id[address] = i;
      d[i]++;
    }
    // LOG(ERROR)<<" txn :"<<i<<" dep:"<<d[i];
  }

  std::queue<std::unique_ptr<ExecutionContext> > qq;

  int process_num = 0;
  int max_process = 0;
  // LOG(ERROR)<<" verify size:"<<ranklist.size();
  for (int i = 0; i < ranklist.size(); ++i) {
    if (d[i] == 0) {
      // auto context = std::make_unique<ExecutionContext>(*ranklist[i].first);
      // context->GetContractExecuteInfo()->commit_id = i;
      // request_queue_.Push(std::move(context));
      x_request_queue_.Push(std::make_unique<int>(i));
      // qq.push(std::move(context));
      // LOG(ERROR)<<" start verify:"<<i;
      // done.insert(i);
      process_num++;
      max_process = std::max(max_process, process_num);
    }
  }

  /*
  while(!qq.empty()){
    auto p = std::move(qq.front());
    qq.pop();
  }

  return true;
  */

  // LOG(ERROR)<<"total num:"<<process_num;
  bool fail = false;
  while (process_num > 0) {
    auto resp = resp_queue_.Pop();
    if (resp == nullptr) {
      continue;
    }
    process_num--;
    int resp_commit_id = resp->commit_id;
    // LOG(ERROR)<<"commit:"<<resp_commit_id;
    assert(resp_commit_id < d.size());
    //  assert(done.find(resp_commit_id) != done.end());
    //  done.erase(done.find(resp_commit_id));
    bool ret = controller_->Commit(resp_commit_id);
    if (!ret) {
      // LOG(ERROR)<<"verify fail:"<<resp_commit_id;
      assert(1 == 0);
      fail = true;
      continue;
    }
    //  LOG(ERROR)<<"verify done:"<<resp_commit_id;
    /*
      auto * v_rws = controller_->GetChangeList(resp_commit_id);

      if(!RWSEqual(*rws, *ranklist[resp_commit_id].second)){
        LOG(ERROR)<<"rws not equal";
        return false;
      }
      */
    for (int n_id : g[resp_commit_id]) {
      d[n_id]--;
      if (d[n_id] == 0) {
        // auto context =
        // std::make_unique<ExecutionContext>(*ranklist[n_id].first);
        // context->GetContractExecuteInfo()->commit_id = n_id;
        // request_queue_.Push(std::move(context));
        x_request_queue_.Push(std::make_unique<int>(n_id));
        //   done.insert(n_id);
        process_num++;
        max_process = std::max(max_process, process_num);
        //  LOG(ERROR)<<"next verify id:"<<n_id<<" process:"<<process_num;
      }
    }
  }
  // LOG(ERROR)<<"verify done:"<<max_process;
  // assert(done.empty());
  return true;
}

bool XVerifier::RWSEqual(const ModifyMap& a, const ModifyMap& b) {
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
