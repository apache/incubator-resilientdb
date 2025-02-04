#pragma once

#include <memory>
#include <string>
#include <thread>

#include "core/db.h"
#include "core/properties.h"
#include "platform/common/queue/lock_free_queue.h"
#include "service/contract/benchmark/ycsb/db/contract_executor.h"
#include "service/contract/executor/common/contract_execute_info.h"

namespace resdb {
namespace contract {
namespace benchmark {

class ContractDB : public ycsbc::DB {
 public:
  ContractDB(int batch_size);
  virtual ~ContractDB();

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields, std::vector<KVPair> &result);

  int Scan(const std::string &table, const std::string &key, int len,
           const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) {
    throw "Scan: function not implemented!";
  }

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return Update(table, key, values);
  }

  int Delete(const std::string &table, const std::string &key) {
    return ycsbc::DB::kOK;
  }

  void Wait();
  std::vector<uint64_t> GetLatencys();
  std::vector<int> GetRetryTime();

 protected:
  std::string ConvertToInt(const std::string &value);

  virtual bool Process();

  virtual Address GetAccountAddress();
  virtual Address GetContractAddress();

  struct QueryItem {
   public:
    uint64_t start_time;
    ContractExecuteInfo info;
    QueryItem(uint64_t start_time, const ContractExecuteInfo &info)
        : start_time(start_time), info(info){};
  };

  void AddLatency(uint64_t latency);
  void AddRetryTime(int retry_time);

 protected:
  std::unique_ptr<ContractExecutor> contract_executor_;

 private:
  LockFreeQueue<QueryItem> request_;
  bool is_stop_;
  std::thread process_thread_;
  std::mutex mutex_;
  std::map<std::string, uint64_t> data_;
  std::atomic<uint64_t> local_id_;
  uint64_t idx_;
  bool finish_ = false;
  std::vector<uint64_t> latency_list_;
  std::vector<int> retry_time_;
  std::map<uint64_t, uint64_t> times_;
  uint64_t user_id_;
  int batch_size_;
};

}  // namespace benchmark
}  // namespace contract
}  // namespace resdb
