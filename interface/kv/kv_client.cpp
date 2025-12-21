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

#include "interface/kv/kv_client.h"

#include <glog/logging.h>

namespace resdb {

KVClient::KVClient(const ResDBConfig& config)
    : TransactionConstructor(config) {}

int KVClient::Set(const std::string& key, const std::string& data) {
  KVRequest request;
  request.set_cmd(KVRequest::SET);
  request.set_key(key);
  request.set_value(data);
  return SendRequest(request);
}

std::unique_ptr<std::string> KVClient::Get(const std::string& key) {
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

std::unique_ptr<std::string> KVClient::GetAllValues() {
  KVRequest request;
  request.set_cmd(KVRequest::GETALLVALUES);
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

int KVClient::Set(const std::string& key, const std::string& data,
                  int version) {
  KVRequest request;
  request.set_cmd(KVRequest::SET_WITH_VERSION);
  request.set_key(key);
  request.set_value(data);
  request.set_version(version);
  return SendRequest(request);
}

std::unique_ptr<ValueInfo> KVClient::Get(const std::string& key, int version) {
  KVRequest request;
  request.set_cmd(KVRequest::GET_WITH_VERSION);
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
  request.set_cmd(KVRequest::GET_KEY_RANGE);
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
  request.set_cmd(KVRequest::GET_HISTORY);
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
  request.set_cmd(KVRequest::GET_TOP);
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
