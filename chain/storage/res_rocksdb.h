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

#pragma once

#include <optional>
#include <string>

#include "chain/storage/storage.h"
#include "platform/proto/replica_info.pb.h"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"

namespace resdb {

std::unique_ptr<Storage> NewResRocksDB(
    const char* cert_file, std::optional<resdb::ResConfigData> config_data);

class ResRocksDB : public Storage {
 public:
  ResRocksDB(const char* cert_file, std::optional<ResConfigData> config_data);
  virtual ~ResRocksDB();
  int SetValue(const std::string& key, const std::string& value) override;
  std::string GetValue(const std::string& key) override;
  std::string GetAllValues(void) override;
  std::string GetRange(const std::string& min_key,
                       const std::string& max_key) override;

  bool Flush() override;

 private:
  std::unique_ptr<rocksdb::DB> db_ = nullptr;
  rocksdb::WriteBatch batch_;
  unsigned int num_threads_ = 1;
  unsigned int write_buffer_size_ = 64 << 20;
  unsigned int write_batch_size_ = 1;
};

}  // namespace resdb
