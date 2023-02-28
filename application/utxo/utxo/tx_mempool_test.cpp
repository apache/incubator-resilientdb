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

#include "application/utxo/utxo/tx_mempool.h"

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
