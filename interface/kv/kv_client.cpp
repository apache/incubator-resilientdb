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

#include "interface/kv/kv_client.h"

#include <glog/logging.h>

namespace resdb {

KVClient::KVClient(const ResDBConfig& config)
    : TransactionConstructor(config) {}

int KVClient::Set(const std::string& key, const std::string& data) {
  KVRequest request;
  request.set_cmd(Operation::SET);
  request.set_key(key);
  request.set_value(data);
  return SendRequest(request);
}

std::unique_ptr<std::string> KVClient::Get(const std::string& key) {
  KVRequest request;
  request.set_cmd(Operation::GET);
  request.set_key(key);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<std::string>(response.value());
}

std::unique_ptr<std::string> KVClient::GetAllValues() {
  KVRequest request;
  request.set_cmd(Operation::GETALLVALUES);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<std::string>(response.value());
}

std::unique_ptr<std::string> KVClient::GetRange(const std::string& min_key,
                                                const std::string& max_key) {
  KVRequest request;
  request.set_cmd(Operation::GETRANGE);
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

int KVClient::Set(const std::string& key, const std::string& data,
                  int version) {
  KVRequest request;
  request.set_cmd(Operation::SET_WITH_VERSION);
  request.set_key(key);
  request.set_value(data);
  request.set_version(version);
  return SendRequest(request);
}

std::unique_ptr<ValueInfo> KVClient::Get(const std::string& key, int version) {
  KVRequest request;
  request.set_cmd(Operation::GET_WITH_VERSION);
  request.set_key(key);
  request.set_version(version);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<ValueInfo>(response.value_info());
}

std::unique_ptr<Items> KVClient::GetKeyRange(const std::string& min_key,
                                             const std::string& max_key) {
  KVRequest request;
  request.set_cmd(Operation::GET_KEY_RANGE);
  request.set_min_key(min_key);
  request.set_max_key(max_key);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<Items>(response.items());
}

std::unique_ptr<Items> KVClient::GetKeyHistory(const std::string& key,
                                               int min_version,
                                               int max_version) {
  KVRequest request;
  request.set_cmd(Operation::GET_HISTORY);
  request.set_key(key);
  request.set_min_version(min_version);
  request.set_max_version(max_version);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<Items>(response.items());
}

std::unique_ptr<Items> KVClient::GetKeyTopHistory(const std::string& key,
                                                  int top_number) {
  KVRequest request;
  request.set_cmd(Operation::GET_TOP);
  request.set_key(key);
  request.set_top_number(top_number);
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return nullptr;
  }
  return std::make_unique<Items>(response.items());
}

}  // namespace resdb
