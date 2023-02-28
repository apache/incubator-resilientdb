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

#include "application/utxo/proto/config.pb.h"
#include "application/utxo/utxo/transaction.h"
#include "application/utxo/utxo/wallet.h"
#include "config/resdb_config_utils.h"
#include "execution/custom_query.h"
#include "execution/transaction_executor_impl.h"

namespace resdb {
namespace utxo {

class UTXOExecutor : public TransactionExecutorImpl {
 public:
  UTXOExecutor(const Config& config, Transaction* transaction, Wallet* wallet);
  ~UTXOExecutor();

  std::unique_ptr<std::string> ExecuteData(const std::string& request) override;

 private:
  Transaction* transaction_;
  Wallet* wallet_;
};

class QueryExecutor : public CustomQuery {
 public:
  QueryExecutor(Transaction* transaction, Wallet* wallet);
  virtual ~QueryExecutor() = default;

  virtual std::unique_ptr<std::string> Query(
      const std::string& request_str) override;

 private:
  Transaction* transaction_;
  Wallet* wallet_;
};

}  // namespace utxo
}  // namespace resdb
