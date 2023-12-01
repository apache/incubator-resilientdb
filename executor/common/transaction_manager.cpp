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

#include "executor/common/transaction_manager.h"

#include <glog/logging.h>

namespace resdb {

TransactionManager::TransactionManager(bool is_out_of_order, bool need_response)
    : is_out_of_order_(is_out_of_order), need_response_(need_response) {}

bool TransactionManager::IsOutOfOrder() { return is_out_of_order_; }

bool TransactionManager::NeedResponse() { return need_response_; }

std::unique_ptr<std::string> TransactionManager::ExecuteData(
    const std::string& request) {
  return std::make_unique<std::string>();
}

std::unique_ptr<BatchUserResponse> TransactionManager::ExecuteBatch(
    const BatchUserRequest& request) {
  std::unique_ptr<BatchUserResponse> batch_response =
      std::make_unique<BatchUserResponse>();
  for (auto& sub_request : request.user_requests()) {
    std::unique_ptr<std::string> response =
        ExecuteData(sub_request.request().data());
    if (response == nullptr) {
      response = std::make_unique<std::string>();
    }
    batch_response->add_response()->swap(*response);
  }

  return batch_response;
}

}  // namespace resdb
