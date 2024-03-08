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

#include <memory>

#include "chain/storage/storage.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

/*
 * TransactionManager is an interface to manager the transactions that have been
 * committed by the replicas. ExecuteData will be called inside ExecuteBatch to
 * execute each transaction. One of the interface ExecuteBatch or ExecuteData
 * should be implemented, but not both.
 */
class TransactionManager {
 public:
  TransactionManager(bool is_out_of_order = false, bool need_response = true);
  virtual ~TransactionManager() = default;

  virtual std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request);

  virtual std::unique_ptr<std::string> ExecuteData(const std::string& request);

  bool IsOutOfOrder();

  bool NeedResponse();

  virtual Storage* GetStorage() { return nullptr; };

 private:
  bool is_out_of_order_ = false;
  bool need_response_ = true;
};

}  // namespace resdb
