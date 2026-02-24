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

#include "platform/consensus/ordering/poc/pow/merkle.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/format.hpp>

#include "common/test/test_macros.h"
#include "platform/consensus/ordering/poc/pow/miner_utils.h"

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
