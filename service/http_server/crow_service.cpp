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
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include <unordered_set>
#include <mutex>


using crow::request;
using crow::response;
using resdb::ResDBConfig;
using resdb::BatchUserRequest;

namespace sdk {

CrowService::CrowService(ResDBConfig config, ResDBConfig server_config,
                         uint16_t port_num)
    : config_(config),
      server_config_(server_config),
      port_num_(port_num),
      kv_client_(config_),
      txn_client_(server_config_) {}

void CrowService::run() {
  crow::SimpleApp app;

  // For adding and removing websocket connections
  std::mutex mtx;

  // Get all values
  CROW_ROUTE(app, "/v1/transactions")
  ([this](const crow::request& req, response& res) {
    auto values = kv_client_.GetValues();
    if (values != nullptr) {
      LOG(INFO) << "client getvalues value = " << values->c_str();

      // Send updated blocks list to websocket
      if (users.size() > 0) {
        for (auto u : users)
          u->send_text("Update blocks");
      }

      res.set_header("Content-Type", "application/json");
      res.end(std::string(values->c_str()));
    } else {
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("getvalues fail");
    }
  });

  // Get value of specific id
  CROW_ROUTE(app, "/v1/transactions/<string>")
  ([this](const crow::request& req, response& res, std::string id) {
    auto value = kv_client_.Get(id);
    if (value != nullptr) {
      LOG(INFO) << "client get value = " << value->c_str();

      // Send updated blocks list to websocket
      if (users.size() > 0) {
        for (auto u : users)
          u->send_text("Update blocks");
      }

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
  ([this](const crow::request& req, response& res, std::string min_id,
          std::string max_id) {
    auto value = kv_client_.GetRange(min_id, max_id);
    if (value != nullptr) {
      LOG(INFO) << "client getrange value = " << value->c_str();

      // Send updated blocks list to websocket
      if (users.size() > 0) {
        for (auto u : users)
          u->send_text("Update blocks");
      }

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
    resdb::SDKTransaction transaction = resdb::ParseSDKTransaction(body);
    const std::string id = transaction.id;
    const std::string value = transaction.value;

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
    response res(201, "id: " + id);  // Created status code
    res.set_header("Content-Type", "text/plain");
    return res;
  });

  CROW_ROUTE(app, "/v1/blocks")([this](const crow::request& req, response& res) {
    auto values = GetAllBlocks(1);
    res.set_header("Content-Type", "application/json");
    res.end(values);
  });

  // Retrieve blocks in batches of size of the int parameter
  CROW_ROUTE(app, "/v1/blocks/<int>")
  ([this](const crow::request& req, response& res, int batch_size) {
    auto values = GetAllBlocks(batch_size);
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
  ([this](const crow::request& req, response& res, int min_seq, int max_seq) {
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
    for (auto& txn : *resp) {
      BatchUserRequest request;
      KVRequest kv_request;

      std::string cur_batch_str = "";
      if (request.ParseFromString(txn.second)) {
        LOG(INFO) << request.DebugString();

        if (!first_iteration) cur_batch_str.append(",");
        first_iteration = false;

        // id
        uint64_t local_id = request.local_id();
        cur_batch_str.append("{\"id\": " + std::to_string(local_id));

        // number
        cur_batch_str.append(", \"number\": \"" + std::to_string(local_id) + "\"");

        // transactions
        cur_batch_str.append(", \"transactions\": [");
        bool first_transaction = true;
        for (auto& sub_req : request.user_requests()) {
          kv_request.ParseFromString(sub_req.request().data());
          std::string kv_request_json = ParseKVRequest(kv_request);

          if (!first_transaction) cur_batch_str.append(",");
          first_transaction = false;
          cur_batch_str.append(kv_request_json);
          cur_batch_str.append("\n");
        }
        cur_batch_str.append("]"); // close transactions list

        // size
        cur_batch_str.append(", \"size\": " + std::to_string(request.ByteSizeLong()));

        // createdAt
        uint64_t createtime = request.createtime();
        cur_batch_str.append(", \"createdAt\": \"" + ParseCreateTime(createtime) + "\"");
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
    .onopen([&](crow::websocket::connection& conn) {
      LOG(INFO) << "Opened websocket";
      std::lock_guard<std::mutex> _(mtx);
      users.insert(&conn);
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
      LOG(INFO) << "Closed websocket";
      std::lock_guard<std::mutex> _(mtx);
      users.erase(&conn);
    })
    .onmessage([&](crow::websocket::connection& /*conn*/, const std::string& data, bool is_binary){
      // do nothing
    });

  // For metadata table on the Explorer
  CROW_ROUTE(app, "/populatetable")
  ([this](const crow::request& req, response& res) {
    std::vector<resdb::ReplicaInfo> replicas = config_.GetReplicaInfos();
    size_t replica_num = replicas[0].id() - 1;
    uint32_t client_num = config_.GetReplicaNum();
    uint32_t worker_num = config_.GetWorkerNum();
    uint32_t client_batch_num = config_.ClientBatchNum();
    uint32_t max_process_txn = config_.GetMaxProcessTxn();
    uint32_t client_batch_wait_time = config_.ClientBatchWaitTimeMS();
    uint32_t input_worker_num = config_.GetInputWorkerNum();
    uint32_t output_worker_num = config_.GetOutputWorkerNum();
    int client_timeout_ms = config_.GetClientTimeoutMs();
    int min_data_receive_num = config_.GetMinDataReceiveNum();
    size_t max_malicious_replica_num = config_.GetMaxMaliciousReplicaNum();
    int checkpoint_water_mark = config_.GetCheckPointWaterMark();

    std::string values = "";
    values.append("[{   \"replicaNum\": " + std::to_string(replica_num) 
                      + ", \"clientNum\": " + std::to_string(client_num)
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
                      + "" "}]");  
    LOG(INFO) <<   std::string(values.c_str());
    res.set_header("Content-Type", "application/json");
    res.end(std::string(values.c_str()));
  });

  // Run the Crow app
  app.port(port_num_).multithreaded().run();
}

// If batch_size is 1, the function will not add the extra outer [] braces
// Otherwise, a list of lists of blocks will be returned 
std::string CrowService::GetAllBlocks(int batch_size) {
  int min_seq = 1;
  bool full_batches = true;

  std::string values = "[\n";
  bool first_batch = true;
  while (full_batches) {
    std::string cur_batch_str = "";
    if (!first_batch) cur_batch_str.append(",\n");
    if (batch_size > 1) cur_batch_str.append("[");
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
    for (auto& txn : *resp) {
      BatchUserRequest request;
      KVRequest kv_request;
      cur_size++;
      if (request.ParseFromString(txn.second)) {
        LOG(INFO) << request.DebugString();

        if (!first_batch_element) cur_batch_str.append(",");
        first_batch_element = false;

        // id
        uint64_t local_id = request.local_id();
        cur_batch_str.append("{\"id\": " + std::to_string(local_id));

        // number
        cur_batch_str.append(", \"number\": \"" + std::to_string(local_id) + "\"");

        // transactions
        cur_batch_str.append(", \"transactions\": [");
        bool first_transaction = true;
        for (auto& sub_req : request.user_requests()) {
          kv_request.ParseFromString(sub_req.request().data());
          std::string kv_request_json = ParseKVRequest(kv_request);

          if (!first_transaction) cur_batch_str.append(",");
          first_transaction = false;
          cur_batch_str.append(kv_request_json);
          cur_batch_str.append("\n");
        }
        cur_batch_str.append("]"); // close transactions list

        // size
        cur_batch_str.append(", \"size\": " + std::to_string(request.ByteSizeLong()));

        // createdAt
        uint64_t createtime = request.createtime();
        cur_batch_str.append(", \"createdAt\": \"" + ParseCreateTime(createtime) + "\"");
      }
      cur_batch_str.append("}\n");
    }
    full_batches = cur_size == batch_size;
    if (batch_size > 1) cur_batch_str.append("]");

    if (cur_size > 0) values.append(cur_batch_str);

    min_seq += batch_size;
  }

  values.append("\n]\n");

  return values;
}

// Helper function used by the blocks endpoints to create JSON strings
std::string CrowService::ParseKVRequest(const KVRequest& kv_request) {
  rapidjson::Document doc;
  if (kv_request.cmd() == 1) {  // SET
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "SET", allocator);
    doc.AddMember("key", kv_request.key(), allocator);
    doc.AddMember("value", kv_request.value(), allocator);
  } else if (kv_request.cmd() == 2) {  // GET
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "GET", allocator);
    doc.AddMember("key", kv_request.key(), allocator);
  } else if (kv_request.cmd() == 3) {  // GETVALUES
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value val(rapidjson::kObjectType);
    doc.AddMember("cmd", "GETVALUES", allocator);
  } else if (kv_request.cmd() == 4) {  // GETRANGE
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
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

  std::tm *tm_gmt = std::gmtime((time_t*) &sec);
  int year = tm_gmt->tm_year + 1900;
  timestr += std::to_string(tm_gmt->tm_mon + 1) + "/" + std::to_string(tm_gmt->tm_mday) + "/" + std::to_string(year) + " ";
  timestr += std::to_string(tm_gmt->tm_hour) + ":" + std::to_string(tm_gmt->tm_min) + ":" + std::to_string(tm_gmt->tm_sec) + " GMT";

  return timestr;
}


}  // namespace resdb
