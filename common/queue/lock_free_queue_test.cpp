#include "common/queue/lock_free_queue.h"

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
