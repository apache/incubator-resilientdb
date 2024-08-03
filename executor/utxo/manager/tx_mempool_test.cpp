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

#include "executor/utxo/manager/tx_mempool.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace utxo {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::Test;

TEST(TxMempoolTest, AddTx) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");
  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  int64_t value = pool.GetUTXOOutValue(0, 0, "0000");
  EXPECT_EQ(value, 100);
}

TEST(TxMempoolTest, AddTxNoIdx) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");

  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  int64_t value = pool.GetUTXOOutValue(1, 1, "0000");
  EXPECT_EQ(value, -1);
}

TEST(TxMempoolTest, AddTxNoOutAddr) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");

  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  int64_t value = pool.GetUTXOOutValue(1, 0, "0002");
  EXPECT_EQ(value, -1);
}

TEST(TxMempoolTest, AddTxNoAddr) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");

  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  int64_t value = pool.GetUTXOOutValue(2, 0, "0000");
  EXPECT_EQ(value, -1);
}

TEST(TxMempoolTest, AddTxMarkSpent) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");

  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  int64_t value = pool.MarkSpend(0, 0, "0000");
  EXPECT_EQ(value, 100);

  EXPECT_EQ(pool.GetUTXOOutValue(0, 0, "0000"), -1);
}

TEST(TxMempoolTest, AddTxAndGet) {
  UTXO utxo;
  auto out = utxo.add_out();
  out->set_value(100);
  out->set_address("0000");

  utxo.set_address("0001");

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo), 0);

  auto value_or = pool.GetUTXO(0, 0, "0000");
  EXPECT_TRUE(value_or.ok());

  EXPECT_EQ((*value_or).value(), 100);
}

TEST(TxMempoolTest, GetList) {
  UTXO utxo1;
  UTXO utxo2;
  UTXO utxo3;

  {
    auto out = utxo1.add_out();
    out->set_value(100);
    out->set_address("0002");
    utxo1.set_address("0001");
  }

  {
    auto in = utxo2.add_in();
    in->set_prev_id(0);

    auto out = utxo2.add_out();
    out->set_value(100);
    out->set_address("0003");
    utxo2.set_address("0002");
  }
  {
    auto in = utxo3.add_in();
    in->set_prev_id(1);

    auto out = utxo3.add_out();
    out->set_value(100);
    out->set_address("0004");
    utxo3.set_address("0003");
  }

  TxMempool pool;
  EXPECT_EQ(pool.AddUTXO(utxo1), 0);
  EXPECT_EQ(pool.AddUTXO(utxo2), 1);
  EXPECT_EQ(pool.AddUTXO(utxo3), 2);

  utxo1.set_transaction_id(0);
  utxo2.set_transaction_id(1);
  utxo3.set_transaction_id(2);
  {
    auto list = pool.GetUTXO(-1, 2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_THAT(list[0], EqualsProto(utxo2));
    EXPECT_THAT(list[1], EqualsProto(utxo3));
  }
  {
    auto list = pool.GetUTXO(-1, 3);
    EXPECT_EQ(list.size(), 3);
    EXPECT_THAT(list[0], EqualsProto(utxo1));
    EXPECT_THAT(list[1], EqualsProto(utxo2));
    EXPECT_THAT(list[2], EqualsProto(utxo3));
  }
  {
    auto list = pool.GetUTXO(1, 2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_THAT(list[0], EqualsProto(utxo1));
    EXPECT_THAT(list[1], EqualsProto(utxo2));
  }
  {
    auto list = pool.GetUTXO(2, 4);
    EXPECT_EQ(list.size(), 3);
    EXPECT_THAT(list[0], EqualsProto(utxo1));
    EXPECT_THAT(list[1], EqualsProto(utxo2));
    EXPECT_THAT(list[2], EqualsProto(utxo3));
  }
}

}  // namespace
}  // namespace utxo
}  // namespace resdb
