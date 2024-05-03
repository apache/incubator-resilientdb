/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <map>
#include <optional>
#include <unordered_map>

#include "chain/storage/storage.h"
#include "executor/common/transaction_manager.h"
#include "proto/kv/kv.pb.h"

namespace resdb {

class StrictExecutor : public TransactionManager {
 public:
  StrictExecutor(std::unique_ptr<Storage> storage);
  virtual ~StrictExecutor() = default;

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;
  std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override;

 protected:
  virtual void Set(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  std::string GetAllValues();
  std::string GetRange(const std::string& min_key, const std::string& max_key);

  void SetWithVersion(const std::string& key, const std::string& value,
                      int version);
  void GetWithVersion(const std::string& key, int version, ValueInfo* info);
  void GetAllItems(Items* items);
  void GetKeyRange(const std::string& min_key, const std::string& max_key,
                   Items* items);
  void GetHistory(const std::string& key, int min_key, int max_key,
                  Items* items);
  void GetTopHistory(const std::string& key, int top_number, Items* items);
  void WorkerThread();

 private:
  std::unique_ptr<Storage> storage_;
  std::unordered_map<std::string, int> lock_map_;
  std::mutex lockMutex_;
  std::priority_queue request_queue_;
  bool is_stop_ = false;
  int thread_count_;
  vector<thread> thread_list_;
};

}  // namespace resdb
