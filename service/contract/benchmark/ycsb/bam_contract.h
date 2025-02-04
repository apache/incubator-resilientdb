#pragma once

#include <memory>
#include <string>

#include "core/db.h"
#include "core/properties.h"
#include "service/contract/benchmark/executor/benchmark_executor.h"

namespace resdb {
namespace contract {
namespace benchmark {

class BamContractDB {
 public:
  static std::unique_ptr<::ycsbc::DB> CreateDB(utils::Properties &props);
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
