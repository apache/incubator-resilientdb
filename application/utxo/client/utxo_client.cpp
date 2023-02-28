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

#include "application/utxo/client/utxo_client.h"

#include <glog/logging.h>

#include "application/utxo/proto/rpc.pb.h"

namespace resdb {
namespace utxo {

UTXOClient::UTXOClient(const ResDBConfig& config) : ResDBUserClient(config) {}

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
  for (const auto utxo : utxo_response.utxos()) {
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
