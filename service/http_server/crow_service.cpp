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

#include "service/http_server/crow_service.h"

#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include <mutex>
#include <unordered_set>

using crow::request;
using crow::response;
using resdb::BatchUserRequest;
using resdb::ResDBConfig;

namespace sdk {

CrowService::CrowService(ResDBConfig client_config, ResDBConfig server_config,
                         uint16_t port_num)
    : client_config_(client_config), server_config_(server_config), port_num_(port_num),
      kv_client_(client_config_), txn_client_(server_config_) {
  GetAllBlocks(100, true, false);
}

void CrowService::run() {
  crow::SimpleApp app;

  // For adding and removing websocket connections
  std::mutex mtx;

  // Get all values
  CROW_ROUTE(app, "/v1/transactions")
  ([this](const crow::request &req, response &res) {
    uint64_t cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();

    if (cur_time < last_db_scan_time + DB_SCAN_TIMEOUT_MS) {
      res.code = 503;
      res.set_header("Content-Type", "text/plain");
      res.end("Get all transactions functionality on cooldown (" + 
        std::to_string(last_db_scan_time + DB_SCAN_TIMEOUT_MS -  cur_time) +
        " ms left)");
    } else {
      last_db_scan_time = cur_time;

      auto values = kv_client_.GetAllValues();
      if (values != nullptr) {
        LOG(INFO) << "client getallvalues value = " << values->c_str();

        // Send updated blocks list to websocket
        if (users.size() > 0) {
          for (auto u : users)
            u->send_text("Update blocks");
        }

        num_transactions_++;

        res.set_header("Content-Type", "application/json");
        res.end(std::string(values->c_str()));
      } else {
        res.code = 500;
        res.set_header("Content-Type", "text/plain");
        res.end("getallvalues fail");
      }
    }
  });

  // Get value of specific id
  CROW_ROUTE(app, "/v1/transactions/<string>")
  ([this](const crow::request &req, response &res, std::string id) {
    auto value = kv_client_.Get(id);
    if (value != nullptr) {
      LOG(INFO) << "client get value = " << value->c_str();

      // Send updated blocks list to websocket
      if (users.size() > 0) {
        for (auto u : users)
          u->send_text("Update blocks");
      }

      num_transactions_++;

      res.set_header("Content-Type", "application/json");
      res.end(std::string(value->c_str()));
    } else {
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("get value fail");
    }
  });

  // Get values based on key range
  CROW_ROUTE(app, "/v1/transactions/<string>/<string>")
  ([this](const crow::request &req, response &res, std::string min_id,
          std::string max_id) {
    auto value = kv_client_.GetRange(min_id, max_id);
    if (value != nullptr) {
      LOG(INFO) << "client getrange value = " << value->c_str();

      // Send updated blocks list to websocket
      if (users.size() > 0) {
        for (auto u : users)
          u->send_text("Update blocks");
      }

      num_transactions_++;

      res.set_header("Content-Type", "application/json");
      res.end(std::string(value->c_str()));
    } else {
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("getrange fail");
    }
  });

  // Commit a key-value pair, extracting the id parameter from the JSON
  // object and setting the value as the entire JSON object
  CROW_ROUTE(app, "/v1/transactions/commit")
  .methods("POST"_method)([this](const request& req) {
    std::string body = req.body;
    LOG(INFO) << "body: " << body;

    // Parse transaction JSON
    rapidjson::Document doc;
    doc.Parse(body.c_str());
    if (!doc.IsObject() || !doc.HasMember("id")) {
      response res(400, "Invalid transaction format"); // Bad Request
      res.set_header("Content-Type", "text/plain");
      return res;
    }
    const std::string id = doc["id"].GetString();
    const std::string value = body;

    // Set key-value pair in kv server
    int retval = kv_client_.Set(id, value);

    if (retval != 0) {
      LOG(ERROR) << "Error when trying to commit id " << id;
      response res(500, "id: " + id);
      res.set_header("Content-Type", "text/plain");
      return res;
    }
    LOG(INFO) << "Set " << id << " to " << value;

    // Send updated blocks list to websocket
    if (users.size() > 0) {
      for (auto u : users)
        u->send_text("Update blocks");
    }

    num_transactions_++;

    response res(201, "id: " + id);  // Created status code
    res.set_header("Content-Type", "text/plain");
    return res;
  });

  CROW_ROUTE(app, "/v1/blocks")
  ([this](const crow::request &req, response &res) {
    auto values = GetAllBlocks(100);
    res.set_header("Content-Type", "application/json");
    res.end(values);
  });

  // Retrieve blocks in batches of size of the int parameter
  CROW_ROUTE(app, "/v1/blocks/<int>")
  ([this](const crow::request &req, response &res, int batch_size) {
    auto values = GetAllBlocks(batch_size, false, true);
    if (values == "") {
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("get replica state fail");
      exit(1);
    };
    res.set_header("Content-Type", "application/json");
    res.end(values);
  });

  // Retrieve blocks within a range
  CROW_ROUTE(app, "/v1/blocks/<int>/<int>")
  ([this](const crow::request &req, response &res, int min_seq, int max_seq) {
    auto resp = txn_client_.GetTxn(min_seq, max_seq);
    absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
        uint64_t min_seq, uint64_t max_seq);
    if (!resp.ok()) {
      LOG(ERROR) << "get replica state fail";
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("get replica state fail");
      exit(1);
    }

    std::string values = "[\n";
    bool first_iteration = true;
    for (auto &txn : *resp) {
      BatchUserRequest request;
      KVRequest kv_request;

      std::string cur_batch_str = "";
      if (request.ParseFromString(txn.second)) {
        LOG(INFO) << request.DebugString();

        if (!first_iteration)
          cur_batch_str.append(",");
        first_iteration = false;

        // id
        uint64_t seq = txn.first;
        cur_batch_str.append("{\"id\": " + std::to_string(seq));

        // number
        cur_batch_str.append(", \"number\": \"" + std::to_string(seq) + "\"");

        // transactions
        cur_batch_str.append(", \"transactions\": [");
        bool first_transaction = true;
        for (auto &sub_req : request.user_requests()) {
          kv_request.ParseFromString(sub_req.request().data());
          std::string kv_request_json = ParseKVRequest(kv_request);

          if (!first_transaction)
            cur_batch_str.append(",");
          first_transaction = false;
          cur_batch_str.append(kv_request_json);
          cur_batch_str.append("\n");
        }
        cur_batch_str.append("]"); // close transactions list

        // size
        cur_batch_str.append(", \"size\": " +
                             std::to_string(request.ByteSizeLong()));

        // createdAt
        uint64_t createtime = request.createtime();
        cur_batch_str.append(", \"createdAt\": \"" +
                             ParseCreateTime(createtime) + "\"");
      }
      cur_batch_str.append("}\n");
      values.append(cur_batch_str);
    }
    values.append("]\n");
    res.set_header("Content-Type", "application/json");
    res.end(values);
  });

  CROW_ROUTE(app, "/blockupdatelistener")
      .websocket()
      .onopen([&](crow::websocket::connection &conn) {
        LOG(INFO) << "Opened websocket";
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
      })
      .onclose(
          [&](crow::websocket::connection &conn, const std::string &reason) {
            LOG(INFO) << "Closed websocket";
            std::lock_guard<std::mutex> _(mtx);
            users.erase(&conn);
          })
      .onmessage([&](crow::websocket::connection & /*conn*/,
                     const std::string &data, bool is_binary) {
        // do nothing
      });

  // For metadata table on the Explorer
  CROW_ROUTE(app, "/populatetable")
  ([this](const crow::request& req, response& res) {
    uint32_t replica_num = server_config_.GetReplicaNum();
    uint32_t worker_num = server_config_.GetWorkerNum();
    uint32_t client_batch_num = server_config_.ClientBatchNum();
    uint32_t max_process_txn = server_config_.GetMaxProcessTxn();
    uint32_t client_batch_wait_time = server_config_.ClientBatchWaitTimeMS();
    uint32_t input_worker_num = server_config_.GetInputWorkerNum();
    uint32_t output_worker_num = server_config_.GetOutputWorkerNum();
    int client_timeout_ms = server_config_.GetClientTimeoutMs();
    int min_data_receive_num = server_config_.GetMinDataReceiveNum();
    size_t max_malicious_replica_num = server_config_.GetMaxMaliciousReplicaNum();
    int checkpoint_water_mark = server_config_.GetCheckPointWaterMark();

    // Don't read the ledger if first commit time is known
    if (first_commit_time_ == 0) {
      // Get first block in the chain
      auto resp = txn_client_.GetTxn(1, 1);
      // absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
      //     uint64_t min_seq, uint64_t max_seq);
      if (!resp.ok()) {
        LOG(ERROR) << "get replica state fail";
        res.end("get replica state fail");
      };

      for (auto& txn : *resp) {
        BatchUserRequest request;
        KVRequest kv_request;
        if (request.ParseFromString(txn.second)) {
          // transactions
          for (auto& sub_req : request.user_requests()) {
            kv_request.ParseFromString(sub_req.request().data());
            std::string kv_request_json = ParseKVRequest(kv_request);
          }

          // see resilientdb/common/utils/utils.cpp
          first_commit_time_ = request.createtime() / 1000000;
        }
      }
    }

    uint64_t epoch_time = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    uint64_t chain_age = first_commit_time_ == 0 ? 
                          0 : epoch_time - first_commit_time_;

    auto block_num_resp = txn_client_.GetBlockNumbers();
    if (!block_num_resp.ok()) {
      LOG(ERROR) << "get number fail";
      exit(1);
    }

    std::string values = "";
    values.append("[{ \"replicaNum\": " + std::to_string(replica_num)
                      + ", \"workerNum\" : " + std::to_string(worker_num) 
                      + ", \"clientBatchNum\" : " + std::to_string(client_batch_num)
                      + ", \"maxProcessTxn\" : " + std::to_string(max_process_txn) 
                      + ", \"clientBatchWaitTime\" : " + std::to_string(client_batch_wait_time)
                      + ", \"inputWorkerNum\" : " + std::to_string(input_worker_num)
                      + ", \"outputWorkerNum\" : " + std::to_string(output_worker_num)
                      + ", \"clientTimeoutMs\" : " + std::to_string(client_timeout_ms)
                      + ", \"minDataReceiveNum\" : " + std::to_string(min_data_receive_num)
                      + ", \"maxMaliciousReplicaNum\" : " + std::to_string(max_malicious_replica_num)
                      + ", \"checkpointWaterMark\" : " + std::to_string(checkpoint_water_mark)
                      + ", \"transactionNum\" : " + std::to_string(num_transactions_)
                      + ", \"blockNum\" : " + std::to_string(*block_num_resp)
                      + ", \"chainAge\" : " + std::to_string(chain_age)
                      + "}]");
    LOG(INFO) << std::string(values.c_str());
    res.set_header("Content-Type", "application/json");
    res.end(std::string(values.c_str()));
  });

  // Run the Crow app
  app.port(port_num_).multithreaded().run();
}

// If batch_size is 1, the function will not add the extra outer [] braces
// Otherwise, a list of lists of blocks will be returned
std::string CrowService::GetAllBlocks(int batch_size, bool increment_txn_count,
                                      bool make_sublists) {
  int min_seq = 1;
  bool full_batches = true;

  std::string values = "[\n";
  bool first_batch = true;
  while (full_batches) {
    std::string cur_batch_str = "";
    if (!first_batch)
      cur_batch_str.append(",\n");
    if (batch_size > 1 && make_sublists)
      cur_batch_str.append("[");
    first_batch = false;

    int max_seq = min_seq + batch_size - 1;
    auto resp = txn_client_.GetTxn(min_seq, max_seq);
    absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
        uint64_t min_seq, uint64_t max_seq);
    if (!resp.ok()) {
      LOG(ERROR) << "get replica state fail";
      return "";
    };

    int cur_size = 0;
    bool first_batch_element = true;
    for (auto &txn : *resp) {
      BatchUserRequest request;
      KVRequest kv_request;
      cur_size++;
      if (request.ParseFromString(txn.second)) {
        if (!first_batch_element) cur_batch_str.append(",");
        
        first_batch_element = false;

        // id
        uint64_t seq = txn.first;
        cur_batch_str.append("{\"id\": " + std::to_string(seq));

        // number
        cur_batch_str.append(", \"number\": \"" + std::to_string(seq) + "\"");

        // transactions
        cur_batch_str.append(", \"transactions\": [");
        bool first_transaction = true;
        for (auto &sub_req : request.user_requests()) {
          kv_request.ParseFromString(sub_req.request().data());
          std::string kv_request_json = ParseKVRequest(kv_request);

          if (!first_transaction)
            cur_batch_str.append(",");
          first_transaction = false;
          cur_batch_str.append(kv_request_json);
          cur_batch_str.append("\n");

          if (increment_txn_count) num_transactions_++;
        }
        cur_batch_str.append("]"); // close transactions list

        // size
        cur_batch_str.append(", \"size\": " +
                             std::to_string(request.ByteSizeLong()));

        // createdAt
        uint64_t createtime = request.createtime();
        cur_batch_str.append(", \"createdAt\": \"" +
                             ParseCreateTime(createtime) + "\"");
      }
      cur_batch_str.append("}\n");
    }
    full_batches = cur_size == batch_size;
    if (batch_size > 1 && make_sublists)
      cur_batch_str.append("]");

    if (cur_size > 0)
      values.append(cur_batch_str);

    min_seq += batch_size;
  }

  values.append("\n]\n");

  return values;
}

// Helper function used by the blocks endpoints to create JSON strings
std::string CrowService::ParseKVRequest(const KVRequest &kv_request) {
  rapidjson::Document doc;
  if (kv_request.cmd() == 1) { // SET
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "SET", allocator);
    doc.AddMember("key", kv_request.key(), allocator);
    doc.AddMember("value", kv_request.value(), allocator);
  } else if (kv_request.cmd() == 2) { // GET
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "GET", allocator);
    doc.AddMember("key", kv_request.key(), allocator);
  } else if (kv_request.cmd() == 3) { // GETALLVALUES
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "GETALLVALUES", allocator);
  } else if (kv_request.cmd() == 4) { // GETRANGE
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "GETRANGE", allocator);
    doc.AddMember("min_key", kv_request.key(), allocator);
    doc.AddMember("max_key", kv_request.value(), allocator);
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

std::string CrowService::ParseCreateTime(uint64_t createtime) {
  std::string timestr = "";
  uint64_t sec = createtime / 1000000; // see resilientdb/common/utils/utils.cpp

  std::tm *tm_gmt = std::gmtime((time_t *)&sec);
  int year = tm_gmt->tm_year + 1900;
  int month = tm_gmt->tm_mon; // 0-indexed
  int day = tm_gmt->tm_mday;

  std::string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  // Using date time string format to support the Explorer transaction chart
  if (day < 10)
    timestr += "0";
  timestr += std::to_string(day) + " " + months[month] + " " +
             std::to_string(year) + " ";

  std::string hour_str = std::to_string(tm_gmt->tm_hour);
  std::string min_str = std::to_string(tm_gmt->tm_min);
  std::string sec_str = std::to_string(tm_gmt->tm_sec);

  if (tm_gmt->tm_hour < 10)
    hour_str = "0" + hour_str;
  if (tm_gmt->tm_min < 10)
    min_str = "0" + min_str;
  if (tm_gmt->tm_sec < 10)
    sec_str = "0" + sec_str;

  timestr += hour_str + ":" + min_str + ":" + sec_str + " GMT";

  return timestr;
}

} // namespace sdk
