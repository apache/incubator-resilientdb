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

#include "glog/logging.h"
#include "service/contract/benchmark/executor/occ_executor.h"
#include "service/contract/executor/manager/leveldb_d_storage.h"
#include "service/contract/executor/manager/leveldb_storage.h"

namespace resdb {
namespace contract {
namespace benchmark {

OCCExecutor::OCCExecutor(ExecutorType name, bool use_leveldb, int worker_num)
    : name_(name), use_leveldb_(use_leveldb), worker_num_(worker_num) {
  InitManager();
}

Address OCCExecutor::CreateAccount() {
  return AddressManager().CreateRandomAddress();
}

Address OCCExecutor::Deploy(const Address& address,
                            const DeployInfo& deploy_info) {
  Address contract_address = manager_->DeployContract(address, deploy_info);
  assert(contract_address > 0);
  return contract_address;
}

absl::StatusOr<std::string> OCCExecutor::ExecuteOne(
    const Address& caller_address, const Address& contract_address,
    const Params& params) {
  return manager_->ExecContract(caller_address, contract_address, params);
}

std::vector<std::unique_ptr<ExecuteResp>> OCCExecutor::ExecuteMulti(
    std::vector<ContractExecuteInfo>& info) {
  return manager_->ExecContract(info);
}

void OCCExecutor::AsyncExecuteMulti(std::vector<ContractExecuteInfo>& info) {
  manager_->AsyncExecContract(info);
}

void OCCExecutor::SetAsyncCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  manager_->SetExecuteCallBack(std::move(func));
}

uint256_t OCCExecutor::Get(uint256_t key) { return storage_->Load(key).first; }

std::unique_ptr<DataStorage> OCCExecutor::GetStorage() {
  if (name_ == OCC) {
    if (use_leveldb_) {
      return std::make_unique<LevelDBStorage>();
    } else {
      return std::make_unique<DataStorage>();
    }
  } else {
    if (use_leveldb_) {
      return std::make_unique<LevelDBStorage>();
    } else {
      return std::make_unique<DataStorage>();
    }
  }
}

void OCCExecutor::InitManager() {
  if (name_ == OCC) {
    auto tmp_storage = GetStorage();
    storage_ = tmp_storage.get();
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num_, ContractManager::XEO);
  } else {
    auto tmp_storage = GetStorage();
    storage_ = tmp_storage.get();
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num_, ContractManager::MultiStreaming);
  }
}

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
