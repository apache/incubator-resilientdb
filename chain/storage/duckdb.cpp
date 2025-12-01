#include "chain/storage/duckdb.h"

#include <glog/logging.h>

#include <exception>
#include <memory>
#include <string>

#include "duckdb.hpp"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResQL(const std::string& path,
                                  const DuckDBInfo& config) {
  DuckDBInfo cfg = config;
  cfg.set_path(path);

  return std::make_unique<ResQL>(cfg);
}

ResQL::ResQL(const DuckDBInfo& config) : config_(config) {
  std::string path = "/tmp/resql-duckdb";
  if (!config.path().empty()) {
    path = config.path();
  }
  CreateDB(config);
}

ResQL::~ResQL() = default;

void ResQL::CreateDB(const DuckDBInfo& config) {
  std::string db_path = config.path();

  duckdb::DBConfig db_config;

  if (config.has_max_memory()) {
    db_config.SetOption("max_memory", duckdb::Value(config.max_memory()));
  }

  if (db_path.empty()) {
    db_ = std::make_unique<duckdb::DuckDB>(nullptr, &db_config);
  } else {
    db_ = std::make_unique<duckdb::DuckDB>(db_path, &db_config);
  }
}

std::string ResQL::ExecuteSQL(const std::string& sql_string){
    if (sql_string.empty()) {
        return "Error: empty SQL query";
    }

    if (!db_) {
        LOG(ERROR) << "DuckDB is not initialized";
        return "Error: database not initialized";
    }

    try {
        conn_ = std::make_unique<duckdb::Connection> (*db_);
        auto result = conn_->Query(sql_string);

        if (!result) {
            LOG(ERROR) << "Query returned nullptr";
            return "Error: query returned no result";
        }

        if (result->HasError()) {
            LOG(ERROR) << "SQL Error: " << result->GetError();
            return "Error: " + result->GetError();
        }

        std::string response = result->ToString();
        LOG(INFO) << "SQL Result:\n" << response;
        return response;
    } catch (const std::exception& e) {
        LOG(ERROR) << "SQL execution threw: " << e.what();
        return std::string("Error: ") + e.what();
    }
}

}  // namespace storage
}  // namespace resdb
