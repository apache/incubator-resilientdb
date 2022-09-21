#include "ordering/poc/pow/merkle.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/format.hpp>

#include "common/test/test_macros.h"
#include "ordering/poc/pow/miner_utils.h"

namespace resdb {
namespace {

TEST(MerkleTest, GetHashFromOneTxn) {
  BatchClientTransactions transactions;
  transactions.add_transactions()->set_transaction_data("txn_1");
  HashValue root_value = Merkle::MakeHash(transactions);

  EXPECT_EQ(GetDigestHexString(GetHashDigest(root_value)),
            "875c5b1c32e0fd4826f1c8191a2a8abf840936e5a2aef9bba5ac8c4f54c14129");
}

TEST(MerkleTest, GetHash) {
  BatchClientTransactions transactions;
  transactions.add_transactions()->set_transaction_data("txn_1");
  transactions.add_transactions()->set_transaction_data("txn_2");
  HashValue root_value = Merkle::MakeHash(transactions);

  EXPECT_EQ(GetDigestHexString(GetHashDigest(root_value)),
            "c659bb4eed97cf5bcc375df90b78742762ac731be916b19ce7efc8217bf33b0a");
}

TEST(MerkleTest, GetHashFromThreeTxn) {
  BatchClientTransactions transactions;
  transactions.add_transactions()->set_transaction_data("txn_1");
  transactions.add_transactions()->set_transaction_data("txn_2");
  transactions.add_transactions()->set_transaction_data("txn_3");
  HashValue root_value = Merkle::MakeHash(transactions);

  EXPECT_EQ(GetDigestHexString(GetHashDigest(root_value)),
            "1cee75f552290940e8f7b195b5e573fafe3db67eadcea19a76bcecd1df94d8cd");
}

}  // namespace
}  // namespace resdb
