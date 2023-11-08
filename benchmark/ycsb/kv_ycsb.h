#pragma once

#include <string>
#include <memory>
#include "core/db.h"
#include "core/properties.h"
#include "service/contract/benchmark/executor/benchmark_executor.h"

namespace resdb {
namespace benchmark {

class KVDB {
  public:
    static std::unique_ptr<::ycsbc::DB> CreateDB(utils::Properties &props);
};


} // ycsbc
}