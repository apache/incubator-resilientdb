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

#include "common/queue/batch_queue.h"

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
