#include "durable_layer/leveldb_durable.h"

#include <assert.h>
#include <glog/logging.h>

using leveldb::DB;
using leveldb::Options;
using leveldb::Status;
using leveldb::WriteOptions;

LevelDurable::LevelDurable(char* cert_file,
                           std::optional<ResConfigData> config_data) {
  if (cert_file == NULL) {
    LOG(ERROR) << "No cert file provided, generating default path";
    path_ = "/tmp/nexres-leveldb-0";
    LOG(ERROR) << "Database path: " << path_;
  } else {
    LOG(ERROR) << "Cert file: " << cert_file;
    std::string str(cert_file);
    path_ = "/tmp/nexres-leveldb-";
    std::string directory_id = "";
    for (int i = 0; i < (int)str.size(); i++) {
      if (str[i] >= '1' && str[i] <= '9') {
        directory_id += std::string(1, cert_file[i]);
      }
    }
    if (directory_id == "") {
        directory_id = "0";
      }
    path_ += directory_id;
    LOG(ERROR) << "Database path: " << path_;
  }

  if (config_data.has_value()) {
    LevelDBInfo config = (*config_data).leveldb_info();
    write_buffer_size_ = config.write_buffer_size_mb() << 20;
    write_batch_size_ = config.write_batch_size();
    if (config.path() != "") {
      LOG(ERROR) << "Custom path for LevelDB provided in config: "
                 << config.path();
      path_ = config.path();
    }
  }
  LOG(ERROR) << "LevelDB Settings: " << write_buffer_size_ << " "
             << write_batch_size_;

  leveldb::Options options;
  options.create_if_missing = true;
  options.write_buffer_size = write_buffer_size_;

  leveldb::DB* db = nullptr;
  status_ = leveldb::DB::Open(options, path_, &db);
  if (status_.ok()) {
    db_ = std::unique_ptr<leveldb::DB>(db);
  }

  LOG(ERROR) << "Successfully opened LevelDB";
}

LevelDurable::LevelDurable(void) {
  path_ = "/tmp/nexres-leveldb-0";
  LOG(ERROR) << "No constructor params given, generating default path "
             << path_;
  leveldb::Options options;
  options.create_if_missing = true;
  options.write_buffer_size = write_buffer_size_;

  leveldb::DB* db = nullptr;
  status_ = leveldb::DB::Open(options, path_, &db);
  if (status_.ok()) {
    db_ = std::unique_ptr<leveldb::DB>(db);
  }
  LOG(ERROR) << "Successfully opened LevelDB";
}

void LevelDurable::setDurable(const std::string& key,
                              const std::string& value) {
  batch_.Put(key, value);

  if (batch_.ApproximateSize() >= write_batch_size_) {
    status_ = db_->Write(WriteOptions(), &batch_);
    batch_.Clear();
  }
}

std::string LevelDurable::getDurable(const std::string& key) {
  std::string value = "";
  status_ = db_->Get(leveldb::ReadOptions(), key, &value);
  return value;
}

LevelDurable::~LevelDurable() { db_.reset(); }
