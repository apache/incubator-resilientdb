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

#include "platform/consensus/ordering/poc/pow/merkle.h"

#include "platform/consensus/ordering/poc/pow/miner_utils.h"

namespace resdb {
namespace {

std::string TrverseMakeHash(const BatchClientTransactions& transaction,
                            int l_idx, int r_idx) {
  if (l_idx == r_idx) {
    return GetHashValue(transaction.transactions(l_idx).transaction_data());
  }
  int mid = (l_idx + r_idx) >> 1;

  std::string l_chd = TrverseMakeHash(transaction, l_idx, mid);
  std::string r_chd = TrverseMakeHash(transaction, mid + 1, r_idx);
  return GetHashValue(l_chd + r_chd);
}

}  // namespace

HashValue Merkle::MakeHash(const BatchClientTransactions& transaction) {
  std::string root_hash =
      TrverseMakeHash(transaction, 0, transaction.transactions_size() - 1);
  return DigestToHash(root_hash);
}

}  // namespace resdb
