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

#include "executor/utxo/manager/transaction.h"

#include <glog/logging.h>

#include <iomanip>
#include <sstream>

#include "common/crypto/hash.h"
#include "common/crypto/signature_utils.h"

namespace resdb {
namespace utxo {

Transaction::Transaction(const Config& config, Wallet* wallet)
    : config_(config), wallet_(wallet) {
  tx_mempool_ = std::make_unique<TxMempool>();
  for (const UTXO& trans : config_.genesis_transactions().transactions()) {
    int64_t transaction_id = tx_mempool_->AddUTXO(trans);

    for (const UTXOOut& output : trans.out()) {
      int ret = wallet_->AddCoin(output.address(), output.value());
      if (ret) {
        LOG(ERROR) << "add coin fail";
        assert(ret == 0);
      }
    }

    LOG(ERROR) << "get genesis utxo:" << trans.DebugString()
               << " transaction id:" << transaction_id;
  }
}

Transaction::~Transaction() {}

int64_t Transaction::AddTransaction(const UTXO& utxo) {
  absl::StatusOr<std::vector<UTXOOut>> ins_or = GetInput(utxo);
  if (!ins_or.ok()) {
    LOG(ERROR) << "get input fail:" << ins_or.status().message();
    return -1;
  }

  if (!VerifyUTXO(utxo, *ins_or)) {
    LOG(ERROR) << "transaction is not valid";
    return -1;
  }

  if (AddCoin(utxo)) {
    LOG(ERROR) << "add coin fail";
    return -1;
  }
  return tx_mempool_->AddUTXO(utxo);
}

int64_t Transaction::AddTransaction(const std::string& utxo_str) {
  UTXO utxo;
  if (!utxo.ParseFromString(utxo_str)) {
    LOG(ERROR) << "parse utxo fail";
    return -1;
  }
  return AddTransaction(utxo);
}

absl::StatusOr<std::vector<UTXOOut>> Transaction::GetInput(const UTXO& utxo) {
  std::vector<UTXOOut> utxos;
  if (utxo.in_size() == 0) {
    if (utxo.address() == "0000") {
      return utxos;
    }
  }

  int64_t verify_nonce = 0;
  std::string public_key;
  for (const UTXOIn& input : utxo.in()) {
    absl::StatusOr<UTXOOut> utxo_or =
        tx_mempool_->GetUTXO(input.prev_id(), input.out_idx(), utxo.address());
    if (!utxo_or.ok()) {
      LOG(ERROR) << "get pre utxo id fail:" << input.prev_id();
      return utxo_or.status();
    }
    utxos.push_back(*utxo_or);
    if (public_key.empty()) {
      public_key = (*utxo_or).pub_key();
    }
    verify_nonce += input.prev_id();
  }

  if (utxos.empty()) {
    LOG(ERROR) << "no input";
    return absl::InvalidArgumentError("Input invalid.");
  }

  if (utxo.sig().empty()) {
    LOG(ERROR) << "utxo no sig, invalid";
    return absl::InvalidArgumentError("Input invalid.");
  }

  LOG(ERROR) << "get public key:" << public_key << " nonce:" << verify_nonce;

  bool valid = utils::ECDSAVerifyString(
      utxo.address() + std::to_string(verify_nonce), public_key, utxo.sig());
  if (!valid) {
    LOG(ERROR) << "key not valid";
    return absl::InvalidArgumentError("Key invalid.");
  }

  for (const UTXOOut& utxo : utxos) {
    if (utxo.pub_key() != public_key) {
      LOG(ERROR) << "public key not match";
      return absl::InvalidArgumentError("Key invalid.");
    }
  }

  return utxos;
}

bool Transaction::VerifyUTXO(const UTXO& utxo,
                             const std::vector<UTXOOut>& inputs) {
  int64_t total_input_v = 0;
  for (const UTXOOut& input : inputs) {
    int64_t value = input.value();
    total_input_v += value;
  }

  for (const UTXOOut& output : utxo.out()) {
    total_input_v -= output.value();
  }

  if (total_input_v <= 0) {
    LOG(ERROR) << "input is not enough";
    return false;
  }

  return true;
}

int64_t Transaction::GetUTXOOutValue(int64_t transaction_id, int out_idx,
                                     const std::string& address) {
  return tx_mempool_->GetUTXOOutValue(transaction_id, out_idx, address);
}

int Transaction::AddCoin(const UTXO& utxo) {
  int64_t total_value = 0;
  for (const UTXOIn& input : utxo.in()) {
    int64_t value = tx_mempool_->MarkSpend(input.prev_id(), input.out_idx(),
                                           utxo.address());
    assert(value >= 0);
    total_value += value;
  }

  int ret = wallet_->AddCoin(utxo.address(), -total_value);
  if (ret) {
    LOG(ERROR) << "add coin fail";
    return -1;
  }

  for (const UTXOOut& output : utxo.out()) {
    int ret = wallet_->AddCoin(output.address(), output.value());
    if (ret) {
      LOG(ERROR) << "add coin fail";
      return -1;
    }
  }
  return 0;
}

std::vector<UTXO> Transaction::GetUTXO(int64_t end_id, int num) {
  return tx_mempool_->GetUTXO(end_id, num);
}

}  // namespace utxo
}  // namespace resdb
