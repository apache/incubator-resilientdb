#include "durable_layer/rocksdb_durable.h"

#include <assert.h>
#include <glog/logging.h>

using rocksdb::DB;
using rocksdb::Options;
using rocksdb::PinnableSlice;
using rocksdb::ReadOptions;
using rocksdb::WriteBatch;
using rocksdb::WriteOptions;

RocksDurable::RocksDurable(char* cert_file,
                           std::optional<ResConfigData> config_data) {
  if (cert_file == NULL) {
    LOG(ERROR) << "No cert file provided, generating default path";
    path_ = "/tmp/nexres-rocksdb-0";
    LOG(ERROR) << "Database path: " << path_;
  } else {
    LOG(ERROR) << "Cert file: " << cert_file;
    std::string str(cert_file);
    path_ = "/tmp/nexres-rocksdb-";
    std::string directory_id = "";
    for (int i = 0; i < (int)str.size(); i++)
      if (str[i] >= '1' && str[i] <= '9')
        directory_id += std::string(1, cert_file[i]);

    if (directory_id == "") directory_id = "0";
    path_ += directory_id;
    LOG(ERROR) << "Database path: " << path_;
  }

  if (config_data.has_value()) {
    RocksDBInfo config = (*config_data).rocksdb_info();
    num_threads_ = config.num_threads();
    write_buffer_size_ = config.write_buffer_size_mb() << 20;
    write_batch_size_ = config.write_batch_size();
    if (config.path() != "") {
      LOG(ERROR) << "Custom path for RocksDB provided in config: "
                 << config.path();
      path_ = config.path();
    }
  }
  LOG(ERROR) << "RocksDB Settings: " << num_threads_ << " "
             << write_buffer_size_ << " " << write_batch_size_;

  rocksdb::Options options;
  options.create_if_missing = true;
  if (num_threads_ > 1) options.IncreaseParallelism(num_threads_);
  options.OptimizeLevelStyleCompaction();
  options.write_buffer_size = write_buffer_size_;

  rocksdb::DB* db = nullptr;
  db_status_ = rocksdb::DB::Open(options, path_, &db);
  if (db_status_.ok()) {
    db_ = std::unique_ptr<rocksdb::DB>(db);
  }
  LOG(ERROR) << "Successfully opened RocksDB";
}

RocksDurable::RocksDurable(void) {
  path_ = "/tmp/nexres-rocksdb-0";
  LOG(ERROR) << "No constructor params given, generating default path "
             << path_;
  rocksdb::Options options;
  options.create_if_missing = true;
  if (num_threads_ > 1) options.IncreaseParallelism(num_threads_);
  options.OptimizeLevelStyleCompaction();
  options.write_buffer_size = write_buffer_size_;

  rocksdb::DB* db = nullptr;
  db_status_ = rocksdb::DB::Open(options, path_, &db);
  if (db_status_.ok()) {
    db_ = std::unique_ptr<rocksdb::DB>(db);
  }
  LOG(ERROR) << "Successfully opened RocksDB";
}

void RocksDurable::setDurable(const std::string& key,
                              const std::string& value) {
  batch_.Put(key, value);

  if (batch_.Count() >= write_batch_size_) {
    db_->Write(WriteOptions(), &batch_);
    batch_.Clear();
  }
}

std::string RocksDurable::getDurable(const std::string& key) {
  std::string value = "";
  db_->Get(ReadOptions(), key, &value);
  return value;
}

RocksDurable::~RocksDurable() { db_.reset(); }
