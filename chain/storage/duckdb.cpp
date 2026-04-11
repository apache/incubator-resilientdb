/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "chain/storage/duckdb.h"

#include <glog/logging.h>

#include <exception>
#include <memory>
#include <string>

#include "duckdb.hpp"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewDuckDB(const std::string& path,
                                  const DuckDBInfo& config) {
  DuckDBInfo cfg = config;
  cfg.set_path(path);

  return std::make_unique<DuckDB>(cfg);
}

DuckDB::DuckDB(const DuckDBInfo& config) : config_(config) {
  std::string path = "/tmp/resql-duckdb";
  if (!config.path().empty()) {
    path = config.path();
  }
  CreateDB(config);
}

DuckDB::~DuckDB() = default;

void DuckDB::CreateDB(const DuckDBInfo& config) {
  std::string db_path = config.path();

  duckdb::DBConfig db_config;

  if (config.has_max_memory()) {
    db_config.SetOption("max_memory", duckdb::Value(config.max_memory()));
  }

  LOG(INFO) << "Initializing DuckDB at "
            << (db_path.empty() ? "in-memory" : db_path)
            << " (autoload/autoinstall extensions enabled)";

  if (db_path.empty()) {
    db_ = std::make_unique<duckdb::DuckDB>(nullptr, &db_config);
  } else {
    db_ = std::make_unique<duckdb::DuckDB>(db_path, &db_config);
  }

  // Ensure extension auto-install/load is enabled at the connection level too.
  try {
    duckdb::Connection init_conn(*db_);
    init_conn.Query("SET autoload_known_extensions=1");
    init_conn.Query("SET autoinstall_known_extensions=1");
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to set DuckDB extension auto-load/install flags: "
               << e.what();
  }
}

std::string DuckDB::ExecuteSQL(const std::string& sql_string){
    if (sql_string.empty()) {
        return "Error: empty SQL query";
    }

    if (!db_) {
        LOG(ERROR) << "DuckDB is not initialized";
        return "Error: database not initialized";
    }

    try {
        LOG(INFO) << "Executing SQL: " << sql_string;
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
        LOG(INFO) << "SQL succeeded. Rows: " << result->RowCount()
                  << " Cols: " << result->ColumnCount();
        LOG(INFO) << "SQL Result:\n" << response;
        return response;
    } catch (const std::exception& e) {
        LOG(ERROR) << "SQL execution threw: " << e.what();
        return std::string("Error: ") + e.what();
    }
}

}  // namespace storage
}  // namespace resdb
