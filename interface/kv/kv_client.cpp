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

#include "proto/kv/kv.pb.h"

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

std::unique_ptr<std::string> KVClient::GetValues() {
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

int KVClient::Set(
    const std::vector<std::pair<std::string, std::string>>& key_pairs) {
  KVRequest request;
  for (const std::pair<std::string, std::string>& p : key_pairs) {
    auto* op = request.add_ops();
    op->set_cmd(KVRequest::SET);
    op->set_key(p.first);
    op->set_value(p.second);
  }
  return SendRequest(request);
}

std::vector<std::pair<std::string, std::string>> KVClient::Get(
    const std::vector<std::string>& keys) {
  KVRequest request;
  for (const std::string& k : keys) {
    auto* op = request.add_ops();
    op->set_cmd(KVRequest::GET);
    op->set_key(k);
  }
  KVResponse response;
  int ret = SendRequest(request, &response);
  if (ret != 0) {
    LOG(ERROR) << "send request fail, ret:" << ret;
    return std::vector<std::pair<std::string, std::string>>();
  }

  std::vector<std::pair<std::string, std::string>> resp;
  for (const auto& info : response.resp_info()) {
    resp.push_back(std::make_pair(info.key(), info.value()));
  }
  return resp;
}

}  // namespace resdb
