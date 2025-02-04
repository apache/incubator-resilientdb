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

#include "service/contract/executor/manager/contract_manager.h"

#include "service/contract/executor/manager/address_manager.h"
#include "service/contract/executor/manager/two_phase_committer.h"
//#include
//"service/contract/executor/manager/sequential_concurrency_committer.h"
#include <glog/logging.h>

#include "eEVM/processor.h"
#include "service/contract/executor/manager/ooo_committer.h"
#include "service/contract/executor/manager/streaming_committer.h"
#include "service/contract/executor/manager/streaming_dq_committer.h"
#include "service/contract/executor/manager/streaming_single_committer.h"
#include "service/contract/executor/manager/two_phase_ooo_committer.h"
#include "service/contract/executor/manager/x_committer.h"
#include "service/contract/executor/manager/x_verifier.h"

namespace resdb {
namespace contract {

ContractManager::ContractManager(std::unique_ptr<DataStorage> storage,
                                 int worker_num, Options op) {
  storage_ = std::move(storage);
  gs_ = std::make_unique<GlobalState>(storage_.get());

  switch (op) {
    case TwoPL:
      committer_ = std::make_unique<TwoPhaseCommitter>(storage_.get(),
                                                       gs_.get(), worker_num);
      break;
    case TwoPLOOO:
      committer_ = std::make_unique<TwoPhaseOOOCommitter>(
          storage_.get(), gs_.get(), worker_num);
      break;
    case OOO:
      committer_ =
          std::make_unique<OOOCommitter>(storage_.get(), gs_.get(), worker_num);
      break;
    case Streaming:
      committer_ = std::make_unique<streaming::StreamingCommitter>(
          storage_.get(), gs_.get(), 500, nullptr, worker_num);
      break;
    case SCC:
      // committer_ =
      // std::make_unique<SequentialConcurrencyCommitter>(storage_.get(),
      // gs_.get(), worker_num);
      break;
    case SingleStreaming:
      committer_ = std::make_unique<streaming::StreamingSingleCommitter>(
          storage_.get(), gs_.get(), worker_num);
      break;
    case MultiStreaming:
      committer_ = std::make_unique<streaming::StreamingDQCommitter>(
          storage_.get(), gs_.get(), 1500, nullptr, worker_num);
      break;
    case XE:
      committer_ =
          std::make_unique<XCommitter>(storage_.get(), gs_.get(), worker_num);
      break;
    case XEO:
      committer_ =
          std::make_unique<XCommitter>(storage_.get(), gs_.get(), worker_num);
      //((XCommitter*)committer_.get())->UseOCC();
      break;
  }

  deployer_ = std::make_unique<ContractDeployer>(committer_.get(), gs_.get());

  verifier_ =
      std::make_unique<XVerifier>(storage_.get(), gs_.get(), worker_num);
}

void ContractManager::SetExecuteCallBack(
    std::function<void(std::unique_ptr<ExecuteResp> resp)> func) {
  committer_->SetExecuteCallBack(std::move(func));
}

Address ContractManager::DeployContract(const Address& owner_address,
                                        const DeployInfo& deploy_info) {
  return deployer_->DeployContract(owner_address, deploy_info);
}

bool ContractManager::DeployContract(const Address& owner_address,
                                     const DeployInfo& deploy_info,
                                     const Address& contract_address) {
  return deployer_->DeployContract(owner_address, deploy_info,
                                   contract_address);
}

absl::StatusOr<eevm::AccountState> ContractManager::GetContract(
    const Address& address) {
  return deployer_->GetContract(address);
}

std::vector<std::unique_ptr<ExecuteResp>> ContractManager::ExecContract(
    std::vector<ContractExecuteInfo>& execute_info) {
  std::map<std::string, std::set<int>> ct;
  std::map<int, int> cct;
  for (int i = 0; i < execute_info.size(); ++i) {
    std::string func_addr =
        deployer_->GetFuncAddress(execute_info[i].contract_address,
                                  execute_info[i].func_params.func_name());
    if (func_addr.empty()) {
      LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
      execute_info[i].contract_address = 0;
      continue;
    }
    execute_info[i].func_addr = func_addr;
    execute_info[i].commit_id = i + 1;
  }

  return committer_->ExecContract(execute_info);
}

void ContractManager::AsyncExecContract(
    std::vector<ContractExecuteInfo>& execute_info) {
  std::map<std::string, std::set<int>> ct;
  std::map<int, int> cct;
  for (int i = 0; i < execute_info.size(); ++i) {
    std::string func_addr =
        deployer_->GetFuncAddress(execute_info[i].contract_address,
                                  execute_info[i].func_params.func_name());
    if (func_addr.empty()) {
      LOG(ERROR) << "no fouction:" << execute_info[i].func_params.func_name();
      execute_info[i].contract_address = 0;
      continue;
    }
    execute_info[i].func_addr = func_addr;
    execute_info[i].commit_id = i + 1;

    /*
    cct[i]=0;
    if(execute_info[i].func_params.param_size()>0){
      for(int j = 0; j < execute_info[i].func_params.param_size()-1; j++){
        auto& p = execute_info[i].func_params.param(j);
        //LOG(ERROR)<<"get idx:"<<i<<" param:"<<p;
        ct[p].insert(i);
      }
    }
    */
  }

  /*
  for(auto it : ct){
    if(it.second.size()>2){
      for(auto id : it.second){
        cct[id]=1;
      }
    }
  }

  int tot = 0, num = 0;
  for(auto x : cct){
    if(x.second==1)num++;
    tot++;
  }
  LOG(ERROR)<<"tot:"<<tot<<" num:"<<num<<" p:"<<(double)num/tot;
  */

  committer_->AsyncExecContract(execute_info);
}

absl::StatusOr<std::string> ContractManager::ExecContract(
    const Address& caller_address, const Address& contract_address,
    const Params& func_param) {
  std::string func_addr =
      deployer_->GetFuncAddress(contract_address, func_param.func_name());
  if (func_addr.empty()) {
    LOG(ERROR) << "no fouction:" << func_param.func_name();
    return absl::InvalidArgumentError("Func not exist.");
  }

  absl::StatusOr<std::string> result = committer_->ExecContract(
      caller_address, contract_address, func_addr, func_param, gs_.get());
  if (result.ok()) {
    return *result;
  }
  return result.status();
}

bool ContractManager::VerifyContract(
    std::vector<ContractExecuteInfo>& ordered_info,
    std::vector<ConcurrencyController ::ModifyMap> rws_list) {
  return true;
  for (int i = 0; i < ordered_info.size(); ++i) {
    std::string func_addr =
        deployer_->GetFuncAddress(ordered_info[i].contract_address,
                                  ordered_info[i].func_params.func_name());
    if (func_addr.empty()) {
      LOG(ERROR) << "no fouction:" << ordered_info[i].func_params.func_name();
      ordered_info[i].contract_address = 0;
      continue;
    }
    ordered_info[i].func_addr = func_addr;
  }
  return verifier_->VerifyContract(ordered_info, rws_list);
}

}  // namespace contract
}  // namespace resdb
