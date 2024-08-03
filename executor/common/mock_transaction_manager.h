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

#include "executor/common/transaction_manager.h"
#include "gmock/gmock.h"

namespace resdb {

class MockTransactionExecutorDataImpl : public TransactionManager {
 public:
  MockTransactionExecutorDataImpl(bool is_out_of_order = false)
      : TransactionManager(is_out_of_order) {}
  MOCK_METHOD(std::unique_ptr<std::string>, ExecuteData, (const std::string&),
              (override));
};

class MockTransactionManager : public MockTransactionExecutorDataImpl {
 public:
  MockTransactionManager(bool is_out_of_order = false)
      : MockTransactionExecutorDataImpl(is_out_of_order) {}
  MOCK_METHOD(std::unique_ptr<BatchUserResponse>, ExecuteBatch,
              (const BatchUserRequest&), (override));
};

}  // namespace resdb
