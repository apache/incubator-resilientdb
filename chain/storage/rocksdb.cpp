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

#include "chain/storage/rocksdb.h"

#include <glog/logging.h>

#include "chain/storage/proto/kv.pb.h"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResRocksDB(const std::string& path,
                                       std::optional<RocksDBInfo> config) {
  if (config == std::nullopt) {
    config = RocksDBInfo();
  }
  (*config).set_path(path);
  return std::make_unique<ResRocksDB>(config);
}

std::unique_ptr<Storage> NewResRocksDB(std::optional<RocksDBInfo> config) {
  return std::make_unique<ResRocksDB>(config);
}

ResRocksDB::ResRocksDB(std::optional<RocksDBInfo> config) {
  std::string path = "/tmp/nexres-rocksdb";
  if (config.has_value()) {
    num_threads_ = (*config).num_threads();
    write_buffer_size_ = (*config).write_buffer_size_mb() << 20;
    write_batch_size_ = (*config).write_batch_size();
    if ((*config).path() != "") {
      LOG(ERROR) << "Custom path for RocksDB provided in config: "
                 << (*config).path();
      path = (*config).path();
    }
  }
  CreateDB(path);
}

void ResRocksDB::CreateDB(const std::string& path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  if (num_threads_ > 1) options.IncreaseParallelism(num_threads_);
  options.OptimizeLevelStyleCompaction();
  options.write_buffer_size = write_buffer_size_;

  rocksdb::DB* db = nullptr;
  rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
  if (status.ok()) {
    db_ = std::unique_ptr<rocksdb::DB>(db);
    LOG(ERROR) << "Successfully opened RocksDB in path: " << path;
  } else {
    LOG(ERROR) << "RocksDB status fail";
  }
  assert(status.ok());
  LOG(ERROR) << "Successfully opened RocksDB";
}

ResRocksDB::~ResRocksDB() {
  if (db_) {
    db_.reset();
  }
}

int ResRocksDB::SetValue(const std::string& key, const std::string& value) {
  batch_.Put(key, value);

  if (batch_.Count() >= write_batch_size_) {
    rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch_);
    if (status.ok()) {
      batch_.Clear();
    } else {
      LOG(ERROR) << "write value fail:" << status.ToString();
      return -1;
    }
  }
  return 0;
}

std::string ResRocksDB::GetValue(const std::string& key) {
  std::string value = "";
  rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
  if (status.ok()) {
    return value;
  } else {
    return "";
  }
}

std::string ResRocksDB::GetAllValues(void) {
  std::string values = "[";
  rocksdb::Iterator* itr = db_->NewIterator(rocksdb::ReadOptions());
  bool first_iteration = true;
  for (itr->SeekToFirst(); itr->Valid(); itr->Next()) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(itr->value().ToString());
  }
  values.append("]");

  delete itr;
  return values;
}

std::string ResRocksDB::GetRange(const std::string& min_key,
                                 const std::string& max_key) {
  std::string values = "[";
  rocksdb::Iterator* itr = db_->NewIterator(rocksdb::ReadOptions());
  bool first_iteration = true;
  for (itr->Seek(min_key); itr->Valid() && itr->key().ToString() <= max_key;
       itr->Next()) {
    if (!first_iteration) values.append(",");
    first_iteration = false;
    values.append(itr->value().ToString());
  }
  values.append("]");

  delete itr;
  return values;
}

bool ResRocksDB::Flush() {
  rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch_);
  if (status.ok()) {
    batch_.Clear();
    return true;
  }
  LOG(ERROR) << "write value fail:" << status.ToString();
  return false;
}
int ResRocksDB::SetValueWithVersion(const std::string& key,
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

std::pair<std::string, int> ResRocksDB::GetValueWithVersion(
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
std::map<std::string, std::pair<std::string, int>> ResRocksDB::GetAllItems() {
  std::map<std::string, std::pair<std::string, int>> resp;

  rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
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

std::map<std::string, std::pair<std::string, int>> ResRocksDB::GetKeyRange(
    const std::string& min_key, const std::string& max_key) {
  std::map<std::string, std::pair<std::string, int>> resp;

  rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
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
std::vector<std::pair<std::string, int>> ResRocksDB::GetHistory(
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
std::vector<std::pair<std::string, int>> ResRocksDB::GetTopHistory(
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
