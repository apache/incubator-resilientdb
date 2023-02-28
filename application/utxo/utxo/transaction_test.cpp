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

#include "application/utxo/utxo/transaction.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "crypto/hash.h"
#include "crypto/key_generator.h"
#include "crypto/signature_utils.h"

namespace resdb {
namespace utxo {
namespace {

using ::testing::Test;

TEST(TransanctionTest, InvalidInputString) {
  Wallet wallet;
  Transaction transaction(Config(), &wallet);
  EXPECT_EQ(transaction.AddTransaction("123"), -1);
}

TEST(TransanctionTest, NoAddress) {
  UTXO utxo;

  UTXOOut* out = utxo.add_out();
  out->set_address("1234");
  out->set_value(1234);

  std::string str;
  utxo.SerializeToString(&str);

  Wallet wallet;
  Transaction transaction(Config(), &wallet);

  EXPECT_EQ(transaction.AddTransaction(str), -1);
}

TEST(TransanctionTest, EmptyInputUtxo) {
  UTXO utxo;

  UTXOOut* out = utxo.add_out();
  out->set_address("1234");
  out->set_value(1234);

  utxo.set_address("0001");

  std::string str;
  utxo.SerializeToString(&str);

  Wallet wallet;
  Transaction transaction(Config(), &wallet);

  EXPECT_EQ(transaction.AddTransaction(str), -1);
}

TEST(TransanctionTest, DefaultUTXO) {
  SecretKey key = KeyGenerator ::GeneratorKeys(SignatureInfo::ECDSA);

  Config config;
  {
    auto* gensis_txn = config.mutable_genesis_transactions();
    UTXO* utxo = gensis_txn->add_transactions();
    UTXOOut* out = utxo->add_out();
    out->set_address("0001");
    out->set_value(1234);
    out->set_pub_key(key.public_key());
  }

  Wallet wallet;
  Transaction transaction(config, &wallet);

  {
    UTXO utxo;
    UTXOIn* in = utxo.add_in();
    in->set_prev_id(0);
    in->set_out_idx(0);

    UTXOOut* out = utxo.add_out();
    out->set_address("1234");
    out->set_value(234);
    utxo.set_address("0001");

    std::string signature = utils::ECDSASignString(
        key.private_key(), utxo.address() + std::to_string(0));
    utxo.set_sig(signature);

    std::string str;
    utxo.SerializeToString(&str);

    EXPECT_EQ(transaction.AddTransaction(str), 1);
  }

  EXPECT_EQ(wallet.GetCoin("1234"), 234);
}

}  // namespace
}  // namespace utxo
}  // namespace resdb
