#pragma once

#include <optional>
#include <string>

#include "config/resdb_config_utils.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/write_batch.h"

using resdb::ResConfigData;
using resdb::RocksDBInfo;

class RocksDurable {
 public:
  RocksDurable(char* cert_file, std::optional<ResConfigData> config_data);
  RocksDurable(void);
  ~RocksDurable();
  void setDurable(const std::string& key, const std::string& value);
  std::string getDurable(const std::string& key);
  std::string getAllValues(void);

 private:
  std::unique_ptr<rocksdb::DB> db_ = nullptr;
  rocksdb::Status db_status_;
  rocksdb::WriteBatch batch_;
  std::string path_;
  unsigned int num_threads_ = 1;
  unsigned int write_buffer_size_ = 64 << 20;
  unsigned int write_batch_size_ = 1;
};
