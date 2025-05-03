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

#include "platform/common/queue/lock_free_queue.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

struct TestItem {
  std::string data;
};

TEST(LockFreeQueueTest, SendAndPop) {
  LockFreeQueue<TestItem> queue("test");
  auto item = std::make_unique<TestItem>();
  item->data = "test";
  queue.Push(std::move(item));

  auto out = queue.Pop();
  assert(out != nullptr);
  EXPECT_EQ(out->data, "test");
}

}  // namespace

}  // namespace resdb
