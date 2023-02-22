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

#include "durable_layer/leveldb_durable.h"

#include <assert.h>
#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <vector>

using leveldb::DB;
using leveldb::Options;
using leveldb::Status;
using leveldb::WriteOptions;

LevelDurable::LevelDurable(char* cert_file,
                           std::optional<ResConfigData> config_data) {
  std::string directory_id = "";

  path_ = "/tmp/nexres-leveldb";
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
    LevelDBInfo config = (*config_data).leveldb_info();
    write_buffer_size_ = config.write_buffer_size_mb() << 20;
    write_batch_size_ = config.write_batch_size();
    if (config.path() != "") {
      LOG(ERROR) << "Custom path for LevelDB provided in config: "
                 << config.path();
      path_ = config.path();
    }
    if (config.generate_unique_pathnames()) {
      LOG(ERROR) << "Adding number to generate unique pathname: "
                 << directory_id;
      path_ += directory_id;
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
    LOG(ERROR) << "Successfully opened LevelDB in path: " << path_;
  } else {
    LOG(ERROR) << "LevelDB status fail";
  }
}

LevelDurable::LevelDurable(void) {
  path_ = "/tmp/nexres-leveldb-test";
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

void LevelDurable::SetValue(const std::string& key, const std::string& value) {
  batch_.Put(key, value);

  if (batch_.ApproximateSize() >= write_batch_size_) {
    status_ = db_->Write(WriteOptions(), &batch_);
    batch_.Clear();
  }
}

std::string LevelDurable::GetValue(const std::string& key) {
  std::string value = "";
  status_ = db_->Get(leveldb::ReadOptions(), key, &value);
  return value;
}

std::string LevelDurable::GetAllValues(void) {
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

std::string LevelDurable::GetRange(const std::string& min_key,
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

LevelDurable::~LevelDurable() { db_.reset(); }
