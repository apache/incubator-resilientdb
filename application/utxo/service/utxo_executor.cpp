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

#include "application/utxo/service/utxo_executor.h"

#include <glog/logging.h>

#include "application/utxo/proto/rpc.pb.h"

namespace resdb {
namespace utxo {

UTXOExecutor::UTXOExecutor(const Config& config, Transaction* transaction,
                           Wallet* wallet)
    : transaction_(transaction), wallet_(wallet) {}

UTXOExecutor::~UTXOExecutor() {}

std::unique_ptr<std::string> UTXOExecutor::ExecuteData(
    const std::string& client_request) {
  UTXORequest utxo_request;
  UTXOResponse response;

  if (!utxo_request.ParseFromString(client_request)) {
    LOG(ERROR) << "parse data fail";
    return nullptr;
  }

  int64_t ret = transaction_->AddTransaction(utxo_request.utxo());
  response.set_ret(ret);
  std::unique_ptr<std::string> ret_str = std::make_unique<std::string>();
  response.SerializeToString(ret_str.get());
  return ret_str;
}

QueryExecutor::QueryExecutor(Transaction* transaction, Wallet* wallet)
    : transaction_(transaction), wallet_(wallet) {}

std::unique_ptr<std::string> QueryExecutor::Query(
    const std::string& request_str) {
  UTXOQuery query;
  if (!query.ParseFromString(request_str)) {
    LOG(ERROR) << "parse query fail";
    return nullptr;
  }

  UTXOQueryResponse resp;
  if (query.query_transaction()) {
    std::vector<UTXO> utxos =
        transaction_->GetUTXO(query.end_id(), query.num());
    for (const UTXO& utxo : utxos) {
      *resp.add_utxos() = utxo;
    }
  } else {
    resp.set_value(wallet_->GetCoin(query.address()));
  }
  std::unique_ptr<std::string> resp_str = std::make_unique<std::string>();
  resp.SerializeToString(resp_str.get());
  return resp_str;
}

}  // namespace utxo
}  // namespace resdb
