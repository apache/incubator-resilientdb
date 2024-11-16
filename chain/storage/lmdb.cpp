#pragma once

#include "chain/storage/lmdb.h"

#include <lmdb++.h>

#include <iostream>
#include <memory>
#include <sstream>

#include "chain/storage/storage.h"

namespace resdb {
namespace storage {

using ValueType = std::pair<std::string, int>;
using ItemsType = std::map<std::string, ValueType>;
using ValuesType = std::vector<ValueType>;

std::unique_ptr<Storage> NewResLmdb(const std::string& path) {
  size_t map_size = 1024UL * 1024UL * 1024UL;
  return std::make_unique<LmdbStorage>(path, map_size);
}

LmdbStorage::LmdbStorage(const std::string& db_path,
                         size_t map_size = 1024UL * 1024UL * 1024UL)
    : env_(lmdb::env::create()), dbi_(lmdb::dbi::open(nullptr, nullptr)) {
  env_.set_mapsize(map_size);
  env_.open(db_path.c_str(), 0, 0664);
  dbi_ = lmdb::dbi::open(env_, nullptr);  // TODO: resolve env_ error
}

LmdbStorage::~LmdbStorage() = default;

int LmdbStorage::SetValue(const std::string& key, const std::string& value) {
  try {
    auto txn = lmdb::txn::begin(env_);
    dbi_.put(txn, key, value);
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
    std::string value;
    if (dbi_.get(txn, key, value)) {
      return value;
    }
    return "";  // Key not found
  } catch (const std::exception& e) {
    std::cerr << "GetValue error: " << e.what() << std::endl;
    return "";
  }
}

std::string LmdbStorage::GetAllValues() {
  std::stringstream result;
  try {
    auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
    auto cursor = lmdb::cursor::open(txn, dbi_);
    std::string key, value;
    while (cursor.get(key, value, MDB_NEXT)) {
      result << key << ":" << value << "\n";
    }
    cursor.close();
    txn.abort();  // Read-only transaction doesn't need commit
  } catch (const std::exception& e) {
    std::cerr << "GetAllValues error: " << e.what() << std::endl;
  }
  return result.str();
}

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

bool LmdbStorage::Flush() {
  // LMDB writes directly to disk, so explicit flush is unnecessary.
  return true;
}
}  // namespace storage
}  // namespace resdb