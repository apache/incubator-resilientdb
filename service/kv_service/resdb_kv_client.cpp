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

#include "service/kv_service/resdb_kv_client.h"

#include <glog/logging.h>

#include "service/kv_service/proto/kv_server.pb.h"

namespace sdk {

ResDBKVClient::ResDBKVClient(const resdb::ResDBConfig& config)
    : TransactionConstructor(config) {}

int ResDBKVClient::Set(const std::string& key, const std::string& data) {
  KVRequest request;
  request.set_cmd(KVRequest::SET);
  request.set_key(key);
  request.set_value(data);
  return SendRequest(request);
}

std::unique_ptr<std::string> ResDBKVClient::Get(const std::string& key) {
  KVRequest request;
  request.set_cmd(KVRequest::GET);
  request.set_key(key);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<std::string>(response.value());
}

std::unique_ptr<std::string> ResDBKVClient::GetValues() {
  KVRequest request;
  request.set_cmd(KVRequest::GETVALUES);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<std::string>(response.value());
}

std::unique_ptr<std::string> ResDBKVClient::GetRange(
    const std::string& min_key, const std::string& max_key) {
  KVRequest request;
  request.set_cmd(KVRequest::GETRANGE);
  request.set_key(min_key);
  request.set_value(max_key);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<std::string>(response.value());
}

}  // namespace resdb
