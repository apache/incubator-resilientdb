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

#include "service/contract/benchmark/occ_bm.h"
#include "service/contract/executor/manager/d_storage.h"

using namespace resdb;
using namespace resdb::contract;

class OCCPerformanceExecutor : public PerformanceExecutor {
 public:
  OCCPerformanceExecutor(int worker_num, ContractManager::Options op)
      : PerformanceExecutor(worker_num), worker_num_(worker_num), op_(op) {}

  void InitManager() {
    if (op_ == ContractManager::XEO) {
      auto tmp_storage = std::make_unique<D_Storage>();
      storage_ = tmp_storage.get();
      manager_ = std::make_unique<ContractManager>(
          std::move(tmp_storage), worker_num_, ContractManager::XEO);
    } else {
      auto tmp_storage = std::make_unique<D_Storage>();
      storage_ = tmp_storage.get();
      manager_ = std::make_unique<ContractManager>(
          std::move(tmp_storage), worker_num_, ContractManager::MultiStreaming);
    }
  }

 private:
  int worker_num_;
  ContractManager::Options op_;
};

#define OCC "occ"
#define SOCC "socc"

std::string name;

class BMImpl : public OCCBM {
 public:
  BMImpl(bool sync) : OCCBM("trans:" + name, sync) {}

  std::unique_ptr<PerformanceExecutor> GetExecutor(int worker_num) {
    if (name == OCC) {
      return std::make_unique<OCCPerformanceExecutor>(worker_num,
                                                      ContractManager::XEO);
    } else {
      return std::make_unique<OCCPerformanceExecutor>(
          worker_num, ContractManager::MultiStreaming);
    }
  }

  std::string GetContractPath() {
    const std::string data_dir =
        std::string(getenv("PWD")) + "/" + "service/contract/benchmark/";
    return data_dir + "data/ERC20.json";
  }

  std::string GetContractDef() { return "contracts"; }

  std::string GetContractName() { return "ERC20.sol:ERC20Token"; };
};

int main() {
  //  srand(time(0));

  bool use_occ = false;
  if (use_occ) {
    name = OCC;
    BMImpl bm(true);
    bm.Process();
  } else {
    name = SOCC;
    BMImpl bm(false);
    bm.Process();
  }
}
