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

#include "service/contract/benchmark/executor/bam_executor.h"

#include "glog/logging.h"
#include "service/contract/executor/x_manager/leveldb_d_storage.h"
#include "service/contract/executor/x_manager/leveldb_storage.h"

namespace resdb {
namespace contract {
namespace benchmark {

using namespace x_manager;

BamExecutor::BamExecutor(ExecutorType name, bool use_leveldb, int worker_num)
    : name_(name), use_leveldb_(use_leveldb), worker_num_(worker_num) {
  InitManager();
}

Address BamExecutor::CreateAccount() {
  return x_manager::AddressManager().CreateRandomAddress();
}

Address BamExecutor::Deploy(const Address& address,
                            const DeployInfo& deploy_info) {
  Address contract_address = manager_->DeployContract(address, deploy_info);
  assert(contract_address > 0);
  return contract_address;
}

absl::StatusOr<std::string> BamExecutor::ExecuteOne(
    const Address& caller_address, const Address& contract_address,
    const Params& params) {
  return manager_->ExecContract(caller_address, contract_address, params);
}

std::vector<std::unique_ptr<ExecuteResp>> BamExecutor::ExecuteContract(
    std::vector<ContractExecuteInfo>& info) {
  return manager_->ExecContract(info);
}

void BamExecutor::AsyncExecuteMulti(std::vector<ContractExecuteInfo>& info) {
  manager_->AsyncExecContract(info);
}

void BamExecutor::SetAsyncCallBack(
    std::function<void(std::unique_ptr<ExecuteResp>)> func) {
  manager_->SetExecuteCallBack(std::move(func));
}

uint256_t BamExecutor::Get(uint256_t key) { return storage_->Load(key).first; }

std::unique_ptr<DataStorage> BamExecutor::GetStorage() {
  if (name_ == BAMBOO || name_ == XP || name_ == TPL) {
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

void BamExecutor::InitManager() {
  auto tmp_storage = GetStorage();
  storage_ = tmp_storage.get();
  if (name_ == BAMBOO) {
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num_, ContractManager::Options::None);
  } else if (name_ == XP) {
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num_, ContractManager::Options::X);
  } else if (name_ == TPL) {
    manager_ = std::make_unique<ContractManager>(
        std::move(tmp_storage), worker_num_, ContractManager::Options::TwoPL);
  } else {
    /*
       auto tmp_storage = GetStorage();
       storage_ = tmp_storage.get();
       manager_ = std::make_unique<ContractManager>(std::move(tmp_storage),
       worker_num_, ContractManager::X);
     */
  }
}

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
