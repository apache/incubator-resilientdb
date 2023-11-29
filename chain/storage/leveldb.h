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

#include <memory>
#include <optional>
#include <string>

#include "chain/storage/proto/leveldb_config.pb.h"
#include "chain/storage/storage.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

namespace resdb {
namespace storage {

std::unique_ptr<Storage> NewResLevelDB(
    const std::string& path, std::optional<LevelDBInfo> config = std::nullopt);
std::unique_ptr<Storage> NewResLevelDB(
    std::optional<LevelDBInfo> config = std::nullopt);

class ResLevelDB : public Storage {
 public:
  ResLevelDB(std::optional<LevelDBInfo> config_data = std::nullopt);

  virtual ~ResLevelDB();
  int SetValue(const std::string& key, const std::string& value) override;
  std::string GetValue(const std::string& key) override;
  std::string GetAllValues(void) override;
  std::string GetRange(const std::string& min_key,
                       const std::string& max_key) override;

  int SetValueWithVersion(const std::string& key, const std::string& value,
                          int version) override;
  std::pair<std::string, int> GetValueWithVersion(const std::string& key,
                                                  int version) override;

  // Return a map of <key, <value, version>>
  std::map<std::string, std::pair<std::string, int>> GetAllItems() override;
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) override;

  // Return a list of <value, version>
  std::vector<std::pair<std::string, int>> GetHistory(const std::string& key,
                                                      int min_version,
                                                      int max_version) override;

  std::vector<std::pair<std::string, int>> GetTopHistory(
      const std::string& key, int top_number) override;

  bool Flush() override;

 private:
  void CreateDB(const std::string& path);

 private:
  std::unique_ptr<leveldb::DB> db_ = nullptr;
  ::leveldb::WriteBatch batch_;
  unsigned int write_buffer_size_ = 64 << 20;
  unsigned int write_batch_size_ = 1;
};

}  // namespace storage
}  // namespace resdb
