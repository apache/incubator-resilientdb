#pragma once

#include <assert.h>

#include <memory>
#include <optional>
#include <string>

#include "config/resdb_config_utils.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"

using resdb::LevelDBInfo;
using resdb::ResConfigData;
using namespace std;

class LevelDurable {
 public:
  LevelDurable(char* cert_file, std::optional<ResConfigData> config_data);
  LevelDurable(void);
  ~LevelDurable();
  void setDurable(const std::string& key, const std::string& value);
  std::string getDurable(const std::string& key);

 private:
  std::unique_ptr<leveldb::DB> db_ = nullptr;
  leveldb::Options options_;
  leveldb::Status status_;
  leveldb::WriteBatch batch_;
  std::string path_;
  unsigned int write_buffer_size_ = 64 << 20;
  unsigned int write_batch_size_ = 1;
};
