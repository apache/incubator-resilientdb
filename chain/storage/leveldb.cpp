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

#include "chain/storage/leveldb.h"

#include <glog/logging.h>
#include <unistd.h>

#include <cstdint>

#include "chain/storage/proto/kv.pb.h"
#include "leveldb/options.h"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResLevelDB(const std::string& path,
                                       std::optional<LevelDBInfo> config) {
  if (config == std::nullopt) {
    config = LevelDBInfo();
  }
  (*config).set_path(path);
  return std::make_unique<ResLevelDB>(config);
}

std::unique_ptr<Storage> NewResLevelDB(std::optional<LevelDBInfo> config) {
  return std::make_unique<ResLevelDB>(config);
}

ResLevelDB::ResLevelDB(std::optional<LevelDBInfo> config) {
  std::string path = "/tmp/nexres-leveldb";
  if (config.has_value()) {
    write_buffer_size_ = (*config).write_buffer_size_mb() << 20;
    write_batch_size_ = (*config).write_batch_size();
    if (!(*config).path().empty()) {
      LOG(ERROR) << "Custom path for ResLevelDB provided in config: "
                 << (*config).path();
      path = (*config).path();
    }
  }
  if ((*config).enable_block_cache()) {
    uint32_t capacity = 1000;
    if ((*config).has_block_cache_capacity()) {
      capacity = (*config).block_cache_capacity();
    }
    block_cache_ =
        std::make_unique<LRUCache<std::string, std::string>>(capacity);
    LOG(ERROR) << "initialized block cache" << std::endl;
  }
  global_stats_ = Stats::GetGlobalStats();
  CreateDB(path);
}

void ResLevelDB::CreateDB(const std::string& path) {
  LOG(ERROR) << "ResLevelDB Create DB: path:" << path
             << " write buffer size:" << write_buffer_size_
             << " batch size:" << write_batch_size_;
  leveldb::Options options;
  options.create_if_missing = true;
  options.write_buffer_size = write_buffer_size_;

  leveldb::DB* db = nullptr;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);
  if (status.ok()) {
    db_ = std::unique_ptr<leveldb::DB>(db);
  }
  assert(status.ok());
  LOG(ERROR) << "Successfully opened LevelDB";
}

ResLevelDB::~ResLevelDB() {
  if (db_) {
    db_.reset();
  }
  if (block_cache_) {
    block_cache_->Flush();
  }
}

int ResLevelDB::SetValue(const std::string& key, const std::string& value) {
  if (block_cache_) {
    block_cache_->Put(key, value);
  }
  batch_.Put(key, value);

  if (batch_.ApproximateSize() >= write_batch_size_) {
    leveldb::Status status = db_->Write(leveldb::WriteOptions(), &batch_);
    if (status.ok()) {
      batch_.Clear();
      UpdateMetrics();
      return 0;
    } else {
      LOG(ERROR) << "flush buffer fail:" << status.ToString();
      return -1;
    }
  }
  return 0;
}

std::string ResLevelDB::GetValue(const std::string& key) {
  std::string value;
  bool found_in_cache = false;

  if (block_cache_) {
    value = block_cache_->Get(key);
    found_in_cache = !value.empty();
  }

  if (!found_in_cache) {
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    if (!status.ok()) {
      value.clear();  // Ensure value is empty if not found in DB
    }
  }

  UpdateMetrics();
  return value;
}

std::string ResLevelDB::GetAllValues(void) {
  std::string values = "[";
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  bool first_iteration = true;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(it->value().ToString());
  }
  values.append("]");

  delete it;
  return values;
}

std::string ResLevelDB::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  std::string values = "[";
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  bool first_iteration = true;
  for (it->Seek(min_key); it->Valid() && it->key().ToString() <= max_key;
       it->Next()) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(it->value().ToString());
  }
  values.append("]");

  delete it;
  return values;
}

bool ResLevelDB::UpdateMetrics() {
  if (block_cache_ == nullptr) {
    return false;
  }
  std::string stats;
  std::string approximate_size;
  db_->GetProperty("leveldb.stats", &stats);
  db_->GetProperty("leveldb.approximate-memory-usage", &approximate_size);
  global_stats_->SetStorageEngineMetrics(block_cache_->GetCacheHitRatio(),
                                         stats, approximate_size);
  return true;
}

bool ResLevelDB::Flush() {
  leveldb::Status status = db_->Write(leveldb::WriteOptions(), &batch_);
  if (status.ok()) {
    batch_.Clear();
    return true;
  }
  LOG(ERROR) << "flush buffer fail:" << status.ToString();
  return false;
}

int ResLevelDB::SetValueWithVersion(const std::string& key,
                                    const std::string& value, int version) {
  std::string value_str = GetValue(key);
  ValueHistory history;
  if (!history.ParseFromString(value_str)) {
    LOG(ERROR) << "old_value parse fail";
    return -2;
  }

  int last_v = 0;
  if (history.value_size() > 0) {
    last_v = history.value(history.value_size() - 1).version();
  }

  if (last_v != version) {
    LOG(ERROR) << "version does not match:" << version
               << " old version:" << last_v;
    return -2;
  }

  Value* new_value = history.add_value();
  new_value->set_value(value);
  new_value->set_version(version + 1);

  history.SerializeToString(&value_str);
  return SetValue(key, value_str);
}

std::pair<std::string, int> ResLevelDB::GetValueWithVersion(
    const std::string& key, int version) {
  std::string value_str = GetValue(key);
  ValueHistory history;
  if (!history.ParseFromString(value_str)) {
    LOG(ERROR) << "old_value parse fail";
    return std::make_pair("", 0);
  }
  if (history.value_size() == 0) {
    return std::make_pair("", 0);
  }
  if (version > 0) {
    for (int i = history.value_size() - 1; i >= 0; --i) {
      if (history.value(i).version() == version) {
        return std::make_pair(history.value(i).value(),
                              history.value(i).version());
      }
      if (history.value(i).version() < version) {
        break;
      }
    }
  }
  int last_idx = history.value_size() - 1;
  return std::make_pair(history.value(last_idx).value(),
                        history.value(last_idx).version());
}

// Return a map of <key, <value, version>>
std::map<std::string, std::pair<std::string, int>> ResLevelDB::GetAllItems() {
  std::map<std::string, std::pair<std::string, int>> resp;

  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ValueHistory history;
    if (!history.ParseFromString(it->value().ToString()) ||
        history.value_size() == 0) {
      LOG(ERROR) << "old_value parse fail";
      continue;
    }
    const Value& value = history.value(history.value_size() - 1);
    resp.insert(std::make_pair(it->key().ToString(),
                               std::make_pair(value.value(), value.version())));
  }
  delete it;

  return resp;
}

std::map<std::string, std::pair<std::string, int>> ResLevelDB::GetKeyRange(
    const std::string& min_key, const std::string& max_key) {
  std::map<std::string, std::pair<std::string, int>> resp;

  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  for (it->Seek(min_key); it->Valid() && it->key().ToString() <= max_key;
       it->Next()) {
    ValueHistory history;
    if (!history.ParseFromString(it->value().ToString()) ||
        history.value_size() == 0) {
      LOG(ERROR) << "old_value parse fail";
      continue;
    }
    const Value& value = history.value(history.value_size() - 1);
    resp.insert(std::make_pair(it->key().ToString(),
                               std::make_pair(value.value(), value.version())));
  }
  delete it;

  return resp;
}

// Return a list of <value, version>
std::vector<std::pair<std::string, int>> ResLevelDB::GetHistory(
    const std::string& key, int min_version, int max_version) {
  std::vector<std::pair<std::string, int>> resp;
  std::string value_str = GetValue(key);
  ValueHistory history;
  if (!history.ParseFromString(value_str)) {
    LOG(ERROR) << "old_value parse fail";
    return resp;
  }

  for (int i = history.value_size() - 1; i >= 0; --i) {
    if (history.value(i).version() < min_version) {
      break;
    }
    if (history.value(i).version() <= max_version) {
      resp.push_back(
          std::make_pair(history.value(i).value(), history.value(i).version()));
    }
  }

  return resp;
}

// Return a list of <value, version>
std::vector<std::pair<std::string, int>> ResLevelDB::GetTopHistory(
    const std::string& key, int top_number) {
  std::vector<std::pair<std::string, int>> resp;
  std::string value_str = GetValue(key);
  ValueHistory history;
  if (!history.ParseFromString(value_str)) {
    LOG(ERROR) << "old_value parse fail";
    return resp;
  }

  for (int i = history.value_size() - 1;
       i >= 0 && resp.size() < static_cast<size_t>(top_number); --i) {
    resp.push_back(
        std::make_pair(history.value(i).value(), history.value(i).version()));
  }

  return resp;
}

}  // namespace storage
}  // namespace resdb
