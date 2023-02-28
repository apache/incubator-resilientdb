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

#pragma once

#include "absl/status/statusor.h"
#include "application/utxo/proto/config.pb.h"
#include "application/utxo/proto/utxo.pb.h"
#include "application/utxo/utxo/tx_mempool.h"
#include "application/utxo/utxo/wallet.h"

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
