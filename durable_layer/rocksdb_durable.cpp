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

#include "durable_layer/rocksdb_durable.h"

#include <assert.h>
#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using rocksdb::DB;
using rocksdb::Options;
using rocksdb::PinnableSlice;
using rocksdb::ReadOptions;
using rocksdb::WriteBatch;
using rocksdb::WriteOptions;

RocksDurable::RocksDurable(char* cert_file,
                           std::optional<ResConfigData> config_data) {
  std::string directory_id = "";

  path_ = "/tmp/nexres-rocksdb";
  LOG(ERROR) << "Default database path: " << path_;

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
      path_ = config.path();
    }
    if (config.generate_unique_pathnames()) {
      LOG(ERROR) << "Adding number to generate unique pathname: "
                 << directory_id;
      path_ += directory_id;
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
    LOG(ERROR) << "Successfully opened RocksDB in path: " << path_;
  } else {
    LOG(ERROR) << "RocksDB status fail";
  }
}

RocksDurable::RocksDurable(void) {
  path_ = "/tmp/nexres-rocksdb-test";
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

void RocksDurable::SetValue(const std::string& key, const std::string& value) {
  batch_.Put(key, value);

  if (batch_.Count() >= write_batch_size_) {
    db_->Write(WriteOptions(), &batch_);
    batch_.Clear();
  }
}

std::string RocksDurable::GetValue(const std::string& key) {
  std::string value = "";
  db_->Get(ReadOptions(), key, &value);
  return value;
}

std::string RocksDurable::GetAllValues(void) {
  std::string values = "[";
  rocksdb::Iterator* itr = db_->NewIterator(ReadOptions());
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

std::string RocksDurable::GetRange(const std::string& min_key,
                                   const std::string& max_key) {
  std::string values = "[";
  rocksdb::Iterator* itr = db_->NewIterator(ReadOptions());
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

RocksDurable::~RocksDurable() { db_.reset(); }
