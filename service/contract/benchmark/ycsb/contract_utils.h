#pragma once

#include <memory>
#include <string>

#include "core/db.h"
#include "core/properties.h"
#include "service/contract/benchmark/executor/benchmark_executor.h"

namespace resdb {
namespace contract {
namespace benchmark {

ContractExecuteInfo GenerateContractExecuteInfo() {}
/*
  Params func_params;
  func_params.set_func_name("set(address,string,string)");
  func_params.add_param(key);
  func_params.add_param(values[0].first);
  func_params.add_param(values[0].second);

  ContractExecuteInfo info(GetAccountAddress(), GetContractAddress(), "",
  func_params, 0);

  request_.Push(std::make_unique<QueryItem>(1, key, std::vector<std::string>{},
  values));
  */

class OCCContractDB {
 public:
  static std::unique_ptr<::ycsbc::DB> CreateDB(utils::Properties &props);
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
