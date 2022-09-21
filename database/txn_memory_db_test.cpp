#include "database/txn_memory_db.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Pointee;

TEST(TxnMemoryDBTest, GetEmptyValue) {
  TxnMemoryDB db;
  EXPECT_EQ(db.Get(1), nullptr);
}

TEST(TxnMemoryDBTest, GetValue) {
  Request request;
  request.set_seq(1);
  request.set_data("test");

  TxnMemoryDB db;
  db.Put(std::make_unique<Request>(request));
  EXPECT_THAT(db.Get(1), Pointee(EqualsProto(request)));
}

TEST(TxnMemoryDBTest, GetSecondValue) {
  Request request;
  request.set_seq(1);
  request.set_data("test");

  TxnMemoryDB db;
  db.Put(std::make_unique<Request>(request));

  request.set_seq(1);
  request.set_data("test_1");
  db.Put(std::make_unique<Request>(request));

  EXPECT_THAT(db.Get(1), Pointee(EqualsProto(request)));
}

}  // namespace

}  // namespace resdb
