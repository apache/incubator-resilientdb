#pragma once

#include "chain/storage/lmdb.h"

#include <glog/logging.h>
#include <lmdb++.h>

#include <iostream>
#include <memory>

#include "chain/storage/storage.h"

namespace resdb {
namespace storage {

using ValueType = std::pair<std::string, int>;
using ItemsType = std::map<std::string, ValueType>;
using ValuesType = std::vector<ValueType>;

std::unique_ptr<Storage> NewResLmdb(const std::string& path) {
  return std::make_unique<LmdbStorage>(path);
}

LmdbStorage::LmdbStorage(const std::string& db_path,
                         size_t map_size = 1024UL * 1024UL * 1024UL)
    : env_(lmdb::env::create()) {
  env_.set_mapsize(map_size);           // Set maximum size of the database
  env_.open(db_path.c_str(), 0, 0664);  // Open the database
}

LmdbStorage::~LmdbStorage() = default;

int LmdbStorage::SetValue(const std::string& key, const std::string& value) {
  try {
    auto txn = lmdb::txn::begin(env_);
    auto dbi = lmdb::dbi::open(txn, nullptr);
    dbi.put(txn, key, value);
    LOG(ERROR) << value;
    txn.commit();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "SetValue error: " << e.what() << std::endl;
    return -1;
  }
}

std::string LmdbStorage::GetValue(const std::string& key) {
  try {
    auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(txn, nullptr);
    auto cursor = lmdb::cursor::open(txn, dbi);
    std::string nkey, value;
    nkey = key;
    while (cursor.get(nkey, value, MDB_NEXT)) {
      LOG(ERROR) << "value: " << value.c_str();
    }
    cursor.close();
    return value;
  } catch (const std::exception& e) {
    std::cerr << "GetValue error: " << e.what() << std::endl;
    return "";
  }
}

std::string LmdbStorage::GetAllValues() { return ""; }

std::string LmdbStorage::GetRange(const std::string& start,
                                  const std::string& end) {
  return "";
}

int LmdbStorage::SetValueWithVersion(const std::string& key,
                                     const std::string& value, int version) {
  return SetValue(key, value + "|" + std::to_string(version));
}

ValueType LmdbStorage::GetValueWithVersion(const std::string& key,
                                           int version) {
  std::string combined_value = GetValue(key);
  auto delimiter_pos = combined_value.rfind('|');
  if (delimiter_pos != std::string::npos) {
    std::string value = combined_value.substr(0, delimiter_pos);
    int stored_version = std::stoi(combined_value.substr(delimiter_pos + 1));
    if (stored_version == version) {
      return {value, version};
    }
  }
  return {"", -1};
}
// Return a map of <key, <value, version>>
std::map<std::string, std::pair<std::string, int>> LmdbStorage::GetAllItems() {
  std::map<std::string, std::pair<std::string, int>> resp;
  return resp;
}

std::map<std::string, std::pair<std::string, int>> LmdbStorage::GetKeyRange(
    const std::string& min_key, const std::string& max_key) {
  std::map<std::string, std::pair<std::string, int>> resp;

  return resp;
}

// Return a list of <value, version>
std::vector<std::pair<std::string, int>> LmdbStorage::GetHistory(
    const std::string& key, int min_version, int max_version) {
  std::vector<std::pair<std::string, int>> resp;

  return resp;
}

// Return a list of <value, version>
std::vector<std::pair<std::string, int>> LmdbStorage::GetTopHistory(
    const std::string& key, int top_number) {
  std::vector<std::pair<std::string, int>> resp;
  return resp;
}

bool LmdbStorage::Flush() {
  // LMDB writes directly to disk, so explicit flush is unnecessary.
  return true;
}
}  // namespace storage
}  // namespace resdb