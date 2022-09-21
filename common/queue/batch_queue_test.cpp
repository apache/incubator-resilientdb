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
