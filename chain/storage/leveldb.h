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

#include <memory>
#include <optional>
#include <string>

#include "chain/storage/proto/leveldb_config.pb.h"
#include "chain/storage/storage.h"
#include "common/lru/lru_cache.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResLevelDB(
    const std::string& path, std::optional<LevelDBInfo> config = std::nullopt);
std::unique_ptr<Storage> NewResLevelDB(
    std::optional<LevelDBInfo> config = std::nullopt);

class ResLevelDB : public Storage {
 public:
  ResLevelDB(std::optional<LevelDBInfo> config_data = std::nullopt);

  virtual ~ResLevelDB();
  int SetValue(const std::string& key, const std::string& value) override;
  int DelValue(const std::string& key) override;
  std::string GetValue(const std::string& key) override;
  std::string GetAllValues(void) override;
  std::string GetRange(const std::string& min_key,
                       const std::string& max_key) override;

  int SetValueWithVersion(const std::string& key, const std::string& value,
                          int version) override;
  std::pair<std::string, int> GetValueWithVersion(const std::string& key,
                                                  int version) override;

  // Return a map of <key, <value, version>>
  std::map<std::string, std::pair<std::string, int>> GetAllItems() override;
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) override;

  std::vector<std::string> GetKeysByPrefix(const std::string& prefix) override;

  std::vector<std::string> GetKeyRangeByPrefix(const std::string& start_prefix, const std::string& end_prefix) override;

  // Return a list of <value, version>
  std::vector<std::pair<std::string, int>> GetHistory(const std::string& key,
                                                      int min_version,
                                                      int max_version) override;

  std::vector<std::pair<std::string, int>> GetTopHistory(
      const std::string& key, int top_number) override;

  bool UpdateMetrics();

  bool Flush() override;

 private:
  void CreateDB(const std::string& path);

 private:
  std::unique_ptr<leveldb::DB> db_ = nullptr;
  ::leveldb::WriteBatch batch_;
  unsigned int write_buffer_size_ = 64 << 20;
  unsigned int write_batch_size_ = 1;

 protected:
  Stats* global_stats_ = nullptr;
  const leveldb::FilterPolicy* filter_policy_ = nullptr;
  std::unique_ptr<LRUCache<std::string, std::string>> block_cache_;
};

}  // namespace storage
}  // namespace resdb
