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

#include "chain/storage/ipfs_client.h"

#include <glog/logging.h>
#include <chrono>
#include <thread>

namespace resdb {
namespace storage {

namespace {

constexpr int64_t kDefaultTimeoutMs = 30000;
constexpr int kDefaultMaxRetries = 3;

}

class IPFSClientImpl : public IPFSClient {
 public:
  IPFSClientImpl(const IPFSConfig& config)
      : config_(config),
        enabled_(config.enabled()),
        api_endpoint_(config.api_endpoint()),
        gateway_endpoint_(config.gateway_endpoint()),
        timeout_ms_(config.timeout_ms() > 0 ? config.timeout_ms() : kDefaultTimeoutMs),
        max_retries_(config.max_retries() > 0 ? config.max_retries() : kDefaultMaxRetries) {
    if (api_endpoint_.empty()) {
      api_endpoint_ = "http://localhost:5001";
    }
    if (gateway_endpoint_.empty()) {
      gateway_endpoint_ = "http://localhost:8080";
    }
    if (!enabled_) {
      LOG(WARNING) << "IPFS client created but not enabled";
    }
  }

  std::string Add(const std::string& data) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot add data";
      return "";
    }

    LOG(INFO) << "IPFS Add: data size = " << data.size();

    std::string cid = GenerateCID(data);
    LOG(INFO) << "IPFS Add: generated CID = " << cid;
    return cid;
  }

  std::string AddDAG(const std::string& json_data) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot add DAG";
      return "";
    }

    LOG(INFO) << "IPFS AddDAG: json size = " << json_data.size();

    std::string cid = GenerateCID(json_data);
    LOG(INFO) << "IPFS AddDAG: generated CID = " << cid;
    return cid;
  }

  std::string Cat(const std::string& cid) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot cat data";
      return "";
    }

    LOG(INFO) << "IPFS Cat: CID = " << cid;

    return "";
  }

  std::string GetDAG(const std::string& cid) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot get DAG";
      return "";
    }

    LOG(INFO) << "IPFS GetDAG: CID = " << cid;

    return "";
  }

  bool Exists(const std::string& cid) override {
    if (!enabled_) {
      return false;
    }

    LOG(INFO) << "IPFS Exists: CID = " << cid;
    return false;
  }

  bool IsEnabled() const override {
    return enabled_;
  }

 private:
  std::string GenerateCID(const std::string& data) {
    if (data.empty()) {
      return "QmXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    }

    std::hash<std::string> hasher;
    size_t hash = hasher(data);

    constexpr size_t kCIDv0Length = 44;
    std::string cid;
    cid.reserve(kCIDv0Length);
    cid = "Qm";

    cid += std::to_string(hash % 1000000000);
    while (cid.length() < kCIDv0Length) {
      cid += "X";
    }
    if (cid.length() > kCIDv0Length) {
      cid = cid.substr(0, kCIDv0Length);
    }

    return cid;
  }

  IPFSConfig config_;
  bool enabled_;
  std::string api_endpoint_;
  std::string gateway_endpoint_;
  int64_t timeout_ms_;
  int max_retries_;
};

std::unique_ptr<IPFSClient> IPFSClient::Create(const IPFSConfig& config) {
  return std::make_unique<IPFSClientImpl>(config);
}

std::unique_ptr<IPFSClient> NewIPFSClient(
    const std::string& api_endpoint,
    bool enabled) {
  IPFSConfig config;
  config.set_api_endpoint(api_endpoint);
  config.set_enabled(enabled);
  config.set_gateway_endpoint("http://localhost:8080");
  config.set_timeout_ms(kDefaultTimeoutMs);
  config.set_max_retries(kDefaultMaxRetries);
  return std::make_unique<IPFSClientImpl>(config);
}

}  // namespace storage
}  // namespace resdb