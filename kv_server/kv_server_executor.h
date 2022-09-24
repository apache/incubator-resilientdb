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
  void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);

 private:
  std::unordered_map<std::string, std::string> kv_map_;
  LevelDurable l_storage_layer_;
  RocksDurable r_storage_layer_;
  bool equip_rocksdb_ = false;
  bool equip_leveldb_ = false;
};

}  // namespace resdb
