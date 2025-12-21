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

#include "platform/common/queue/batch_queue.h"

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

TEST(BatchQueueTest, NoBatch) {
  BatchQueue<std::unique_ptr<TestItem>> queue("test", 100);
  auto item = std::make_unique<TestItem>();
  item->data = "test";
  queue.Push(std::move(item));

  auto out = queue.Pop(100);
  EXPECT_EQ(out.size(), 1);
  EXPECT_EQ(out[0]->data, "test");
}

TEST(BatchQueueTest, Batch) {
  int tot = 100;
  BatchQueue<std::unique_ptr<TestItem>> queue("test", 100);
  auto th = std::thread([&]() {
    int count = 0;
    while (count != tot) {
      auto out = queue.Pop(100);
      count += out.size();
    }
  });

  for (int i = 0; i < tot; ++i) {
    auto item = std::make_unique<TestItem>();
    item->data = "test";
    queue.Push(std::move(item));
  }
  th.join();
}

TEST(BatchQueueTest, BatchPerformance) {
  int tot = 5000000;
  BatchQueue<std::unique_ptr<TestItem>> queue("test", 100);
  auto th = std::thread([&]() {
    auto start_time = std::chrono::system_clock::now();
    int count = 0;
    int num = 0;
    while (count != tot) {
      auto out = queue.Pop(100);
      count += out.size();
      num++;
    }
    auto current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = (current_time - start_time);
    printf("pop time: %lf num = %d\n", diff.count(), num);
  });
  auto start_time = std::chrono::system_clock::now();
  for (int i = 0; i < tot; ++i) {
    auto item = std::make_unique<TestItem>();
    item->data = "test";
    queue.Push(std::move(item));
  }
  auto current_time = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = (current_time - start_time);
  printf("push time: %lf\n", diff.count());
  printf("queue size %zu\n", queue.Size());
  th.join();
}

}  // namespace

}  // namespace resdb
