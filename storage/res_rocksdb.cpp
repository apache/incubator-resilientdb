/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "storage/res_rocksdb.h"

#include <assert.h>
#include <glog/logging.h>

namespace resdb {

std::unique_ptr<Storage> NewResRocksDB(
    char* cert_file, std::optional<resdb::ResConfigData> config_data) {
  return std::make_unique<ResRocksDB>(cert_file, config_data);
}

ResRocksDB::ResRocksDB(char* cert_file,
                       std::optional<ResConfigData> config_data) {
  std::string directory_id = "";

  std::string path = "/tmp/nexres-rocksdb";
  LOG(ERROR) << "Default database path: " << path;

  if (cert_file == NULL) {
    LOG(ERROR) << "No cert file provided";
  } else {
    LOG(ERROR) << "Cert file: " << cert_file;
    std::string str(cert_file);

    for (int i = 0; i < (int)str.size(); i++) {
      if (str[i] >= '0' && str[i] <= '9') {
        directory_id += std::string(1, cert_file[i]);
      }
    }

    if (directory_id == "") {
      directory_id = "0";
    }
  }

  if (config_data.has_value()) {
    RocksDBInfo config = (*config_data).rocksdb_info();
    num_threads_ = config.num_threads();
    write_buffer_size_ = config.write_buffer_size_mb() << 20;
    write_batch_size_ = config.write_batch_size();
    if (config.path() != "") {
      LOG(ERROR) << "Custom path for RocksDB provided in config: "
                 << config.path();
      path = config.path();
    }
    if (config.generate_unique_pathnames()) {
      LOG(ERROR) << "Adding number to generate unique pathname: "
                 << directory_id;
      path += directory_id;
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
  rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
  if (status.ok()) {
    db_ = std::unique_ptr<rocksdb::DB>(db);
    LOG(ERROR) << "Successfully opened RocksDB in path: " << path;
  } else {
    LOG(ERROR) << "RocksDB status fail";
  }
  assert(status.ok());
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

}  // namespace resdb
