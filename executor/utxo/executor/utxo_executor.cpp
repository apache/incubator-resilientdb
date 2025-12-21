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

#include "proto/utxo/rpc.pb.h"

namespace resdb {
namespace utxo {

UTXOExecutor::UTXOExecutor(const Config& config, Transaction* transaction,
                           Wallet* wallet)
    : transaction_(transaction) {}

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
