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
