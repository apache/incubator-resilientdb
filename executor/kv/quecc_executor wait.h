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

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"
using namespace std;
namespace resdb {
  
struct KVOperation{
  int transaction_number;
  Operation op;
};
class QueccExecutor : public TransactionManager {
 public:
  QueccExecutor(std::unique_ptr<Storage> storage);
  virtual ~QueccExecutor();

  std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request);
  std::unique_ptr<std::string> ExecuteData(const KVOperation& operation);
  std::unique_ptr<std::string> ExecuteData(const KVRequest& kv_request);
  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 protected:
  virtual void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  std::string GetAllValues();
  std::string GetRange(const std::string& min_key, const std::string& max_key);

  void CreateRanges();
  virtual bool VerifyRequest(const std::string& key, const std::string& value) {
    return true;
  }
  void PlannerThread(int thread_number);

 private:
  bool is_out_of_order_ = false;
  bool need_response_ = true;
  std::condition_variable cv_;
  std::condition_variable cv2_;
  std::mutex mutex_;
  std::mutex mutex2_;
  bool is_stop_ = false;
  unordered_map<string, int> rid_to_range_;
  // keeps track of # of operations per txn
  unordered_map<int, int> transaction_tracker_;
  //weight per key
  unordered_map<string, int> key_weight_;
  int total_weight_;
  // Each Planner thread uses its corresponding vector pos
  vector<vector<KVOperation>> batch_array_;
  // sorted_transactions[i] is what planner thread i outputs
  // sorted_transactions[i][j] is range j from planner thread i
  // Iterate through the list at sorted_transactions[i][j] to perform all txns
  vector<vector<vector<KVOperation>>> sorted_transactions_;
  std::vector<KVOperation> operation_list_;
  int thread_count_;
  vector<bool> batch_ready_;
  vector<thread> thread_list_;
  atomic<int> ready_planner_count_;
  std::unique_ptr<BatchUserResponse> batch_response_;
  std::unique_ptr<Storage> storage_;
  vector<KVRequest> multi_op_transactions_;
  vector<int> multi_op_transactions_numbers_;
  vector<vector<KVRequest>> multi_op_batches_;
  vector<vector<int>> multi_op_number_batches_;
  atomic<bool> multi_op_ready_;
  vector<vector<int>> wait_point_list_;
  unordered_map<int, atomic<int>> operations_checked_;
};

}  // namespace resdb
