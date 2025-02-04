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
//#include "service/contract/executor/x_manager/d_storage.h"
#include "service/contract/executor/x_manager/leveldb_storage.h"

using namespace resdb;
using namespace resdb::contract;

#define OCC "occ"
#define SOCC "socc"

std::string name;
bool use_leveldb = false;

enum OP {
  READ = 1,
  WRITE = 2,
  MIX = 3,
  UPDATE = 4,
};

OP op;
int pr;

std::string GetOPName() {
  switch (op) {
    case READ:
      return "get_banlance";
    case WRITE:
      return "reset";
    case UPDATE:
      return "read/write";
    case MIX:
      return "_mix_" + std::to_string(pr);
  }
}

std::string GetName() {
  if (use_leveldb) {
    return "smallbank_db:" + name + GetOPName();
  } else {
    return "smallbank:" + name + GetOPName();
  }
}

class OCCPerformanceExecutor : public PerformanceExecutor {
 public:
  OCCPerformanceExecutor(int worker_num)
      : PerformanceExecutor(worker_num), worker_num_(worker_num) {}

  std::unique_ptr<DataStorage> GetStorage() {
    if (name == OCC) {
      if (use_leveldb) {
        return std::make_unique<LevelDBStorage>();
      } else {
        return std::make_unique<DataStorage>();
      }
    } else {
      if (use_leveldb) {
        // return std::make_unique<LevelDB_D_Storage>();
        return std::make_unique<LevelDBStorage>();
      } else {
        return std::make_unique<DataStorage>();
      }
    }
  }

  void InitManager() {
    if (name == OCC) {
      if (use_leveldb) {
        auto tmp_storage = std::make_unique<LevelDBStorage>();
        storage_ = tmp_storage.get();
        manager_ = std::make_unique<ContractManager>(
            std::move(tmp_storage), worker_num_, ContractManager::XEO);
      } else {
        auto tmp_storage = std::make_unique<DataStorage>();
        storage_ = tmp_storage.get();
        manager_ = std::make_unique<ContractManager>(
            std::move(tmp_storage), worker_num_, ContractManager::XEO);
      }
    } else {
      auto tmp_storage = GetStorage();
      storage_ = tmp_storage.get();
      manager_ = std::make_unique<ContractManager>(
          std::move(tmp_storage), worker_num_, ContractManager::MultiStreaming);
    }
  }

 private:
  int worker_num_;
};

class BMImpl : public OCCBM {
 public:
  BMImpl(bool sync) : OCCBM(GetName(), sync) {}

  std::unique_ptr<PerformanceExecutor> GetExecutor(int worker_num) {
    return std::make_unique<OCCPerformanceExecutor>(worker_num);
  }

  std::string GetContractPath() {
    const std::string data_dir =
        std::string(getenv("PWD")) + "/" + "service/contract/benchmark/";
    return data_dir + "data/smallbank.json";
  }

  std::string GetContractDef() { return "contracts"; }

  std::string GetContractName() { return "smallbank.sol:SmallBank"; }

  Params GetFuncParams(StringGenerator* generator) {
    Params func_params;
    if (op == WRITE) {
      func_params.set_func_name("sendPayment(address,address,uint256)");
      func_params.add_param(generator->Next());
      func_params.add_param(generator->Next());
      func_params.add_param(std::to_string(rand() % 1000));
    } else if (op == UPDATE) {
      func_params.set_func_name("updateBalance(address,uint256)");
      func_params.add_param(generator->Next());
      func_params.add_param(std::to_string(rand() % 1000));
    } else if (op == READ) {
      func_params.set_func_name("getBalance(address)");
      func_params.add_param(generator->Next());
      func_params.add_param(std::to_string(rand() % 1000));
    } else {
      int p = rand() % 100;
      if (p < pr) {
        func_params.set_func_name("getBalance(address)");
        func_params.add_param(generator->Next());
        func_params.add_param(std::to_string(rand() % 1000));
      } else {
        func_params.set_func_name("sendPayment(address,address,uint256)");
        func_params.add_param(generator->Next());
        func_params.add_param(generator->Next());
        func_params.add_param(std::to_string(rand() % 1000));
      }
    }
    return func_params;
  }
};

void Process() {
  if (name == OCC) {
    name = OCC;
    BMImpl bm(true);
    bm.Process();
  } else {
    name = SOCC;
    BMImpl bm(false);
    bm.Process();
  }
}

int main() {
  srand(1234);

  bool use_occ = true;
  use_leveldb = true;
  op = MIX;

  if (use_occ) {
    name = OCC;
  } else {
    name = SOCC;
  }

  if (op == MIX) {
    for (int p : {50}) {
      // for(int p : {5,50,95}){
      pr = p;
      Process();
    }
  } else {
    Process();
  }
}
