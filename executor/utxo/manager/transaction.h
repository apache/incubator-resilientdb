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

#pragma once

#include "absl/status/statusor.h"
#include "executor/utxo/manager/tx_mempool.h"
#include "executor/utxo/manager/wallet.h"
#include "proto/utxo/config.pb.h"
#include "proto/utxo/utxo.pb.h"

namespace resdb {
namespace utxo {

class Transaction {
 public:
  Transaction(const Config& config, Wallet* wallet);
  ~Transaction();

  int64_t AddTransaction(const std::string& utxo_string);
  int64_t AddTransaction(const UTXO& utxo);

  std::vector<UTXO> GetUTXO(int64_t end_id, int num);

 private:
  int AddCoin(const UTXO& utxo);
  bool VerifyUTXO(const UTXO& utxo, const std::vector<UTXOOut>& ins);
  int64_t GetUTXOOutValue(int64_t tx_id, int out_idx,
                          const std::string& address);

  absl::StatusOr<std::vector<UTXOOut>> GetInput(const UTXO& utxo);

 private:
  std::unique_ptr<TxMempool> tx_mempool_;
  Config config_;
  Wallet* wallet_;
};

}  // namespace utxo
}  // namespace resdb
