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

#include "chain/storage/res_leveldb.h"

#include <glog/logging.h>

namespace resdb {

std::unique_ptr<Storage> NewResLevelDB(const char* cert_file,
                                       resdb::ResConfigData config_data) {
  return std::make_unique<ResLevelDB>(cert_file, config_data);
}

ResLevelDB::ResLevelDB(const char* cert_file,
                       std::optional<ResConfigData> config_data) {
  std::string directory_id = "";

  std::string path = "/tmp/nexres-leveldb";
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
      LOG(ERROR) << "Custom path for ResLevelDB provided in config: "
                 << config.path();
      path = config.path();
    }
    if (config.generate_unique_pathnames()) {
      LOG(ERROR) << "Adding number to generate unique pathname: "
                 << directory_id;
      path += directory_id;
    }
  }
  CreateDB(path);
}

void ResLevelDB::CreateDB(const std::string& path) {
  LOG(ERROR) << "ResLevelDB Create DB: path:" << path
             << " write buffer size:" << write_buffer_size_
             << " batch size:" << write_batch_size_;
  leveldb::Options options;
  options.create_if_missing = true;
  options.write_buffer_size = write_buffer_size_;

  leveldb::DB* db = nullptr;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);
  if (status.ok()) {
    db_ = std::unique_ptr<leveldb::DB>(db);
  }
  assert(status.ok());
  LOG(ERROR) << "Successfully opened LevelDB";
}

ResLevelDB::~ResLevelDB() {
  if (db_) {
    db_.reset();
  }
}

int ResLevelDB::SetValue(const std::string& key, const std::string& value) {
  batch_.Put(key, value);

  if (batch_.ApproximateSize() >= write_batch_size_) {
    leveldb::Status status = db_->Write(leveldb::WriteOptions(), &batch_);
    if (status.ok()) {
      batch_.Clear();
      return 0;
    } else {
      LOG(ERROR) << "flush buffer fail:" << status.ToString();
      return -1;
    }
  }
  return 0;
}

std::string ResLevelDB::GetValue(const std::string& key) {
  std::string value = "";
  leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
  if (status.ok()) {
    return value;
  } else {
    LOG(ERROR) << "get value fail:" << status.ToString();
    return "";
  }
}

std::string ResLevelDB::GetAllValues(void) {
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

std::string ResLevelDB::GetRange(const std::string& min_key,
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

//}

}  // namespace resdb
