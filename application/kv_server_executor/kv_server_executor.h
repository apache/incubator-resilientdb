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

#include <map>
#include <optional>
#include <unordered_map>

#include "config/resdb_config_utils.h"
#include "durable_layer/leveldb_durable.h"
#include "durable_layer/rocksdb_durable.h"
#include "execution/transaction_executor_impl.h"

namespace resdb {

class KVServerExecutor : public TransactionExecutorImpl {
 public:
  KVServerExecutor(const ResConfigData& config_data, char* cert_file);
  KVServerExecutor(void);
  virtual ~KVServerExecutor() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 private:
  bool Validate(const std::string& transaction);
  void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  std::string GetValues();
  std::string GetRange(const std::string& min_key, const std::string& max_key);

 private:
  std::unordered_map<std::string, std::string> kv_map_;
  LevelDurable l_storage_layer_;
  RocksDurable r_storage_layer_;
  bool equip_rocksdb_ = false;
  bool equip_leveldb_ = false;
  bool require_txn_validation_;
};

}  // namespace resdb
