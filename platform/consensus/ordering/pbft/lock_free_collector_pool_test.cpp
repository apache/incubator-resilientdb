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

#include "platform/consensus/ordering/pbft/lock_free_collector_pool.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::testing::Test;

class CollectorPoolTest : public Test {
 public:
  int AddRequest(LockFreeCollectorPool* pool, uint64_t seq) {
    Request request;
    request.set_seq(seq);

    TransactionCollector* collector = pool->GetCollector(seq);
    if (collector == nullptr) {
      return 1;
    }
    return collector->AddRequest(
        std::make_unique<Request>(request), SignatureInfo(), false,
        [](const Request&, int, TransactionCollector::CollectorDataType*,
           std::atomic<TransactionStatue>*, bool) {});
  }
};

TEST_F(CollectorPoolTest, AddRequest) {
  LockFreeCollectorPool pool("test", 2, nullptr);
  for (int i = 0; i < 16; ++i) {
    int ret = AddRequest(&pool, i);
    EXPECT_EQ(ret, 0);
  }
  int ret = AddRequest(&pool, 16);
  EXPECT_EQ(ret, -2);
  pool.Update(8);
  EXPECT_EQ(AddRequest(&pool, 16), 0);
}

}  // namespace

}  // namespace resdb
