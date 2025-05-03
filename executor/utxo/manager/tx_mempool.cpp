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

namespace resdb {
namespace utxo {

TxMempool::TxMempool() : id_(0) {}
TxMempool::~TxMempool() {}

int64_t TxMempool::AddUTXO(const UTXO& utxo) {
  LOG(ERROR) << "add utxo id:" << id_;
  int64_t cur_id = id_++;
  txs_[cur_id] = std::make_unique<UTXO>(utxo);
  return cur_id;
}

absl::StatusOr<UTXOOut> TxMempool::GetUTXO(int64_t id, int out_idx,
                                           const std::string& address) {
  if (txs_.find(id) == txs_.end()) {
    LOG(ERROR) << "no utxo:" << id;
    return absl::InvalidArgumentError("id invalid.");
  }

  UTXO* utxo = txs_[id].get();

  if (utxo->out_size() <= out_idx) {
    LOG(ERROR) << " idx not found:" << out_idx << " id:" << id;
    return absl::InvalidArgumentError("id invalid.");
  }

  if (utxo->out(out_idx).spent()) {
    LOG(ERROR) << " value has been spent:" << id << " idx:" << out_idx;
    return absl::InvalidArgumentError("id has been spent.");
  }

  if (utxo->out(out_idx).address() != address) {
    LOG(ERROR) << " address not match:" << address
               << " utxo addr:" << utxo->out(out_idx).address();
    return absl::InvalidArgumentError("address invalid.");
  }
  return utxo->out(out_idx);
}

int64_t TxMempool::GetUTXOOutValue(int64_t id, int out_idx,
                                   const std::string& address) {
  if (txs_.find(id) == txs_.end()) {
    LOG(ERROR) << "no utxo:" << id;
    return -1;
  }

  UTXO* utxo = txs_[id].get();

  if (utxo->out_size() <= out_idx) {
    LOG(ERROR) << " idx not found:" << out_idx << " id:" << id;
    return -1;
  }

  if (utxo->out(out_idx).spent()) {
    LOG(ERROR) << " value has been spent:" << id << " idx:" << out_idx;
    return -1;
  }
  if (utxo->out(out_idx).address() != address) {
    LOG(ERROR) << " address not match:" << address
               << " utxo addr:" << utxo->out(out_idx).address();
    return -1;
  }
  return utxo->out(out_idx).value();
}

int64_t TxMempool::MarkSpend(int64_t id, int out_idx,
                             const std::string& address) {
  if (txs_.find(id) == txs_.end()) {
    LOG(ERROR) << "no utxo:" << id;
    return -1;
  }

  UTXO* utxo = txs_[id].get();
  if (utxo->out_size() <= out_idx) {
    LOG(ERROR) << " idx not found:" << out_idx << " id:" << id;
    return -1;
  }

  utxo->mutable_out(out_idx)->set_spent(true);
  return utxo->out(out_idx).value();
}

std::vector<UTXO> TxMempool::GetUTXO(int64_t end_idx, int num) {
  if (end_idx == -1) {
    int64_t max_id = ((--txs_.end())->first);
    end_idx = max_id;
  }

  std::vector<UTXO> resp;
  for (int i = 0; i < num; ++i) {
    if (txs_.find(end_idx - i) == txs_.end()) {
      break;
    }
    resp.push_back(*txs_[end_idx - i]);
    resp.back().set_transaction_id(end_idx - i);
  }

  std::reverse(resp.begin(), resp.end());

  return resp;
}

}  // namespace utxo
}  // namespace resdb
