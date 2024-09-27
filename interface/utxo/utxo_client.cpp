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

#include "interface/utxo/utxo_client.h"

#include <glog/logging.h>

#include "proto/utxo/rpc.pb.h"

namespace resdb {
namespace utxo {

UTXOClient::UTXOClient(const ResDBConfig& config)
    : TransactionConstructor(config) {}

int UTXOClient::Transfer(const UTXO& utxo) {
  UTXORequest request;
  UTXOResponse response;
  *request.mutable_utxo() = utxo;

  int ret = SendRequest(request, &response);
  if (ret != 0 || response.ret() < 0) {
    return -1;
  }
  return response.ret();
}

std::vector<UTXO> UTXOClient::GetList(int64_t end_id, int num) {
  UTXOQuery query;
  query.set_query_transaction(true);
  query.set_end_id(end_id);
  query.set_num(num);

  CustomQueryResponse response;

  int ret = SendRequest(query, Request::TYPE_CUSTOM_QUERY);
  if (ret) {
    LOG(ERROR) << "send request fail";
    return std::vector<UTXO>();
  }

  ret = RecvRawMessage(&response);
  if (ret) {
    LOG(ERROR) << "recv response fail";
    return std::vector<UTXO>();
  }

  UTXOQueryResponse utxo_response;
  utxo_response.ParseFromString(response.resp_str());
  std::vector<UTXO> utxo_list;
  for (const auto& utxo : utxo_response.utxos()) {
    utxo_list.push_back(utxo);
  }
  return utxo_list;
}

int64_t UTXOClient::GetWallet(const std::string& address) {
  UTXOQuery query;
  query.set_query_transaction(false);
  query.set_address(address);

  CustomQueryResponse response;

  int ret = SendRequest(query, Request::TYPE_CUSTOM_QUERY);
  if (ret) {
    LOG(ERROR) << "send request fail";
    return -1;
  }

  ret = RecvRawMessage(&response);
  if (ret) {
    LOG(ERROR) << "recv response fail";
    return -1;
  }

  UTXOQueryResponse utxo_response;
  utxo_response.ParseFromString(response.resp_str());
  return utxo_response.value();
}

}  // namespace utxo
}  // namespace resdb
