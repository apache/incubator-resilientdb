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

#include "executor/utxo/executor/utxo_executor.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/crypto/key_generator.h"
#include "common/crypto/signature_utils.h"
#include "proto/utxo/rpc.pb.h"

namespace resdb {
namespace utxo {
namespace {

using ::testing::Test;

TEST(UTXOExecutorTest, AddUTXO) {
  Config config;

  SecretKey key = KeyGenerator ::GeneratorKeys(SignatureInfo::ECDSA);

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
  UTXOExecutor txn_manager(config, &transaction, &wallet);

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

    UTXORequest request;
    *request.mutable_utxo() = utxo;

    std::string request_str;
    request.SerializeToString(&request_str);

    auto ret_str = txn_manager.ExecuteData(request_str);
    EXPECT_TRUE(ret_str != nullptr);
  }
}

}  // namespace
}  // namespace utxo
}  // namespace resdb
