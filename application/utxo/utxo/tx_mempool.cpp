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
