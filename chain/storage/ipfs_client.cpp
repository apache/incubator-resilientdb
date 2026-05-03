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

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <glog/logging.h>
#include <regex>
#include <sstream>
#include <thread>

namespace resdb {
namespace storage {

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;

namespace {

constexpr int64_t kDefaultTimeoutMs = 30000;
constexpr int kDefaultMaxRetries = 3;
constexpr char kBoundary[] = "ipfs-client-boundary-1a2b3c4d";

struct EndpointInfo {
  std::string host;
  uint16_t port;
};

EndpointInfo ParseEndpoint(const std::string& url) {
  EndpointInfo info{"localhost", 5001};
  std::string cleaned = url;
  if (cleaned.find("http://") == 0) {
    cleaned = cleaned.substr(7);
  } else if (cleaned.find("https://") == 0) {
    cleaned = cleaned.substr(8);
  }
  auto colon_pos = cleaned.find(':');
  if (colon_pos != std::string::npos) {
    info.host = cleaned.substr(0, colon_pos);
    try {
      info.port = static_cast<uint16_t>(std::stoi(cleaned.substr(colon_pos + 1)));
    } catch (...) {
      info.port = 5001;
    }
  } else {
    info.host = cleaned;
  }
  return info;
}

std::string ExtractHashFromJson(const std::string& json) {
  std::regex hash_regex("\"Hash\"\\s*:\\s*\"([^\"]+)\"");
  std::smatch match;
  if (std::regex_search(json, match, hash_regex) && match.size() > 1) {
    return match[1].str();
  }
  std::regex cid_regex("\"Cid\"\\s*:\\s*\\{\\s*\"/\"\\s*:\\s*\"([^\"]+)\"");
  if (std::regex_search(json, match, cid_regex) && match.size() > 1) {
    return match[1].str();
  }
  return "";
}

std::string BuildMultipartBody(const std::string& data, const std::string& filename = "data") {
  std::ostringstream oss;
  oss << "--" << kBoundary << "\r\n"
      << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filename << "\"\r\n"
      << "Content-Type: application/octet-stream\r\n"
      << "\r\n"
      << data << "\r\n"
      << "--" << kBoundary << "--\r\n";
  return oss.str();
}

class IPFSHttpClient {
 public:
  IPFSHttpClient(const EndpointInfo& endpoint, int64_t timeout_ms, int max_retries)
      : endpoint_(endpoint), timeout_ms_(timeout_ms), max_retries_(max_retries) {
    LOG(INFO) << "IPFS HTTP client initialized: host=" << endpoint.host
              << " port=" << endpoint.port
              << " timeout=" << timeout_ms << "ms"
              << " max_retries=" << max_retries;
  }

  std::string PostRaw(const std::string& path, const std::string& body,
                      const std::string& content_type = "application/octet-stream") {
    beast::error_code ec;
    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> response;

    for (int attempt = 0; attempt <= max_retries_; ++attempt) {
      try {
        tcp::resolver resolver(io_context_);
        tcp::socket socket(io_context_);

        auto const results = resolver.resolve(endpoint_.host,
                                               std::to_string(endpoint_.port), ec);
        if (ec) {
          LOG(ERROR) << "IPFS resolve failed (attempt " << attempt + 1 << "/"
                     << max_retries_ + 1 << "): " << ec.message();
          if (attempt < max_retries_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
          }
          return "";
        }

        beast::get_lowest_layer(socket).connect(*results.begin(), ec);
        if (ec) {
          LOG(ERROR) << "IPFS connect failed (attempt " << attempt + 1 << "/"
                     << max_retries_ + 1 << "): " << ec.message();
          if (attempt < max_retries_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
          }
          return "";
        }

        beast::http::request<beast::http::string_body> req{
            beast::http::verb::post, path, 11};
        req.set(beast::http::field::host,
                endpoint_.host + ":" + std::to_string(endpoint_.port));
        req.set(beast::http::field::content_type, content_type);
        req.set(beast::http::field::user_agent, "resdb-ipfs-client/1.0");
        req.body() = body;
        req.prepare_payload();

        beast::http::write(socket, req, ec);
        if (ec) {
          LOG(ERROR) << "IPFS write failed (attempt " << attempt + 1 << "/"
                     << max_retries_ + 1 << "): " << ec.message();
          if (attempt < max_retries_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
          }
          return "";
        }

        beast::http::read(socket, buffer, response, ec);
        if (ec) {
          LOG(ERROR) << "IPFS read failed (attempt " << attempt + 1 << "/"
                     << max_retries_ + 1 << "): " << ec.message();
          if (attempt < max_retries_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
            continue;
          }
          return "";
        }

        socket.shutdown(tcp::socket::shutdown_both, ec);

        if (response.result() != beast::http::status::ok) {
          LOG(ERROR) << "IPFS HTTP error: " << response.result()
                     << " body: " << response.body();
          return "";
        }

        return response.body();
      } catch (const std::exception& e) {
        LOG(ERROR) << "IPFS request exception (attempt " << attempt + 1 << "/"
                   << max_retries_ + 1 << "): " << e.what();
        if (attempt < max_retries_) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500 * (attempt + 1)));
          continue;
        }
        return "";
      }
    }

    return "";
  }

 private:
  EndpointInfo endpoint_;
  int64_t timeout_ms_;
  int max_retries_;
  asio::io_context io_context_;
};

std::unique_ptr<IPFSHttpClient> CreateHttpClient(const IPFSConfig& config,
                                                  const std::string& endpoint_override = "") {
  std::string endpoint = endpoint_override.empty() ? config.api_endpoint() : endpoint_override;
  if (endpoint.empty()) {
    endpoint = "http://localhost:5001";
  }
  auto info = ParseEndpoint(endpoint);
  int64_t timeout = config.timeout_ms() > 0 ? config.timeout_ms() : kDefaultTimeoutMs;
  int retries = config.max_retries() > 0 ? config.max_retries() : kDefaultMaxRetries;
  return std::make_unique<IPFSHttpClient>(info, timeout, retries);
}

}  // namespace

class IPFSClientImpl : public IPFSClient {
 public:
  explicit IPFSClientImpl(const IPFSConfig& config)
      : config_(config),
        enabled_(config.enabled()) {
    if (!enabled_) {
      LOG(WARNING) << "IPFS client created but not enabled";
    }
    if (config_.api_endpoint().empty()) {
      config_.set_api_endpoint("http://localhost:5001");
    }
    if (config_.gateway_endpoint().empty()) {
      config_.set_gateway_endpoint("http://localhost:8080");
    }
    LOG(INFO) << "IPFS client initialized with endpoint: " << config_.api_endpoint();
  }

  std::string Add(const std::string& data) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot add data";
      return "";
    }

    LOG(INFO) << "IPFS Add: data size = " << data.size();

    std::string body = BuildMultipartBody(data);
    std::string content_type = std::string("multipart/form-data; boundary=") + kBoundary;

    std::string response = PostWithRetry("/api/v0/add", body, content_type);
    if (response.empty()) {
      LOG(ERROR) << "IPFS Add: failed to add data";
      return "";
    }

    std::string cid = ExtractHashFromJson(response);
    if (cid.empty()) {
      LOG(ERROR) << "IPFS Add: could not extract CID from response: " << response;
      return "";
    }

    LOG(INFO) << "IPFS Add: successfully added data, CID = " << cid;
    return cid;
  }

  std::string AddDAG(const std::string& json_data) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot add DAG";
      return "";
    }

    LOG(INFO) << "IPFS AddDAG: json size = " << json_data.size();

    std::string body = BuildMultipartBody(json_data, "dag.json");
    std::string content_type = std::string("multipart/form-data; boundary=") + kBoundary;

    std::string response = PostWithRetry("/api/v0/dag/put", body, content_type);
    if (response.empty()) {
      LOG(ERROR) << "IPFS AddDAG: failed to add DAG node";
      return "";
    }

    std::string cid = ExtractHashFromJson(response);
    if (cid.empty()) {
      LOG(ERROR) << "IPFS AddDAG: could not extract CID from response: " << response;
      return "";
    }

    LOG(INFO) << "IPFS AddDAG: successfully added DAG node, CID = " << cid;
    return cid;
  }

  std::string Cat(const std::string& cid) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot cat data";
      return "";
    }

    LOG(INFO) << "IPFS Cat: CID = " << cid;

    std::string path = std::string("/api/v0/cat?arg=") + cid;
    std::string response = PostWithRetry(path, "", "application/octet-stream");
    if (response.empty()) {
      LOG(ERROR) << "IPFS Cat: failed to retrieve data for CID = " << cid;
      return "";
    }

    LOG(INFO) << "IPFS Cat: successfully retrieved data, size = " << response.size();
    return response;
  }

  std::string GetDAG(const std::string& cid) override {
    if (!enabled_) {
      LOG(ERROR) << "IPFS is not enabled, cannot get DAG";
      return "";
    }

    LOG(INFO) << "IPFS GetDAG: CID = " << cid;

    std::string path = std::string("/api/v0/dag/get?arg=") + cid;
    std::string response = PostWithRetry(path, "", "application/octet-stream");
    if (response.empty()) {
      LOG(ERROR) << "IPFS GetDAG: failed to retrieve DAG node for CID = " << cid;
      return "";
    }

    LOG(INFO) << "IPFS GetDAG: successfully retrieved DAG node";
    return response;
  }

  bool Exists(const std::string& cid) override {
    if (!enabled_) {
      return false;
    }

    LOG(INFO) << "IPFS Exists: CID = " << cid;

    std::string path = std::string("/api/v0/pin/ls?arg=") + cid + "&type=recursive";
    std::string response = PostWithRetry(path, "", "application/octet-stream");
    bool exists = !response.empty() && response.find("\"Type\":\"recursive\"") != std::string::npos;

    if (exists) {
      LOG(INFO) << "IPFS Exists: CID " << cid << " exists";
    } else {
      LOG(INFO) << "IPFS Exists: CID " << cid << " does not exist";
    }
    return exists;
  }

  bool IsEnabled() const override { return enabled_; }

 private:
  std::string PostWithRetry(const std::string& path, const std::string& body,
                            const std::string& content_type) {
    auto client = CreateHttpClient(config_);
    return client->PostRaw(path, body, content_type);
  }

  IPFSConfig config_;
  bool enabled_;
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
