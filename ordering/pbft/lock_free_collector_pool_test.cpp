#include "ordering/pbft/lock_free_collector_pool.h"

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
           std::atomic<TransactionStatue>*) {});
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
