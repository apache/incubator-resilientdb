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
#include "application/utxo/proto/utxo.pb.h"

namespace resdb {
namespace utxo {

class TxMempool {
 public:
  TxMempool();
  ~TxMempool();

  // Add a new utxo and return the transaction id.
  int64_t AddUTXO(const UTXO& utxo);

  // get the transfer value from a transantion "id" in its output[out_idx].
  // if the address not match or the transaction does not exist, return -1
  int64_t GetUTXOOutValue(int64_t id, int out_idx, const std::string& address);

  absl::StatusOr<UTXOOut> GetUTXO(int64_t id, int out_idx,
                                  const std::string& address);

  // Mark the output of a trans has been spent.
  // Return the out value.
  int64_t MarkSpend(int64_t id, int out_idx, const std::string& address);

  std::vector<UTXO> GetUTXO(int64_t end_idx, int num);

 private:
  std::map<int64_t, std::unique_ptr<UTXO> > txs_;
  std::atomic<int64_t> id_;
};

}  // namespace utxo
}  // namespace resdb
