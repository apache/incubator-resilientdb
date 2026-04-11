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
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "chain/storage/proto/duckdb_config.pb.h"
#include "storage.h"
#include "duckdb.hpp"

namespace resdb {
namespace storage {

class DuckDB : public Storage {
 public:
  explicit DuckDB(const DuckDBInfo& config);
  ~DuckDB() override;

  // Main functionality
  std::string ExecuteSQL(const std::string& sql_string) override;
  void CreateDB(const DuckDBInfo& config);

  // ===== Required Storage interface =====

  // Basic KV
  int SetValue(const std::string&, const std::string&) override { return 0; }

  int SetValueWithSeq(const std::string&, const std::string&, uint64_t) override {
    return 0;
  }

  std::string GetValue(const std::string&) override { return ""; }

  std::pair<std::string, uint64_t>
  GetValueWithSeq(const std::string&, uint64_t) override {
    return {"", 0};
  }

  std::string GetRange(const std::string&, const std::string&) override {
    return "";
  }

  // Version-based KV
  int SetValueWithVersion(const std::string&, const std::string&, int) override {
    return 0;
  }

  std::pair<std::string, int>
  GetValueWithVersion(const std::string&, int) override {
    return {"", 0};
  }

  // Full scans
  std::map<std::string,
    std::vector<std::pair<std::string, uint64_t>>>
  GetAllItemsWithSeq() override {
    return {};
  }

  std::map<std::string, std::pair<std::string, int>>
  GetAllItems() override {
    return {};
  }

  std::map<std::string, std::pair<std::string, int>>
  GetKeyRange(const std::string&, const std::string&) override {
    return {};
  }

  // History APIs
  std::vector<std::pair<std::string, int>>
  GetHistory(const std::string&, int, int) override {
    return {};
  }

  std::vector<std::pair<std::string, int>>
  GetTopHistory(const std::string&, int) override {
    return {};
  }

  bool Flush() override { return true; }

 private:
  std::unique_ptr<duckdb::DuckDB> db_;
  std::unique_ptr<duckdb::Connection> conn_;
  std::optional<DuckDBInfo> config_;
};

// Factory function
std::unique_ptr<Storage> NewDuckDB(
    const std::string& path,
    const DuckDBInfo& config = DuckDBInfo());

}  // namespace storage
}  // namespace resdb