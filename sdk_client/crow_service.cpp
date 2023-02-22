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

#include "sdk_client/crow_service.h"

#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <memory>
#include <string>
#include <thread>

using crow::request;
using crow::response;

namespace resdb {

CrowService::CrowService(ResDBConfig config, ResDBConfig server_config,
                         uint16_t port_num)
    : config_(config),
      server_config_(server_config),
      port_num_(port_num),
      kv_client_(config_),
      txn_client_(server_config_) {}

void CrowService::run() {
  crow::SimpleApp app;

  // Get all values
  CROW_ROUTE(app, "/v1/transactions")
  ([this](const crow::request& req, response& res) {
    auto values = kv_client_.GetValues();
    if (values != nullptr) {
      LOG(INFO) << "client getvalues value = " << values->c_str();
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
      res.set_header("Content-Type", "application/json");
      res.end(std::string(value->c_str()));
    } else {
      res.code = 500;
      res.set_header("Content-Type", "text/plain");
      res.end("getrange fail");
    }
  });

  // Set a key-value pair. Accepts JSON objects following the SDK Transaction
  // format with id and value parameters
  CROW_ROUTE(app, "/v1/transactions/commit")
      .methods("POST"_method)([this](const request& req) {
        std::string body = req.body;
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
        response res(201, "id: " + id);  // Created status code
        res.set_header("Content-Type", "text/plain");
        return res;
      });

  // Retrieve blocks in batches of size of the int parameter
  CROW_ROUTE(app, "/v1/blocks/<int>")
  ([this](const crow::request& req, response& res, int batch_size) {
    int min_seq = 1;
    bool full_batches = true;

    std::string values = "[\n";
    bool first_batch = true;
    while (full_batches) {
      std::string cur_batch_str = "";
      if (!first_batch)
        cur_batch_str.append(",\n[");
      else
        cur_batch_str.append("[");
      first_batch = false;

      int max_seq = min_seq + batch_size - 1;
      auto resp = txn_client_.GetTxn(min_seq, max_seq);
      absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
          uint64_t min_seq, uint64_t max_seq);
      if (!resp.ok()) {
        LOG(ERROR) << "get replica state fail";
        res.code = 500;
        res.set_header("Content-Type", "text/plain");
        res.end("get replica state fail");
        exit(1);
      };

      int cur_size = 0;
      bool first_batch_element = true;
      for (auto& txn : *resp) {
        BatchClientRequest request;
        KVRequest kv_request;
        cur_size++;
        if (request.ParseFromString(txn.second)) {
          for (auto& sub_req : request.client_requests()) {
            kv_request.ParseFromString(sub_req.request().data());
            LOG(INFO) << "Block data:\n{\nseq: " << txn.first << "\n"
                      << kv_request.DebugString().c_str() << "}";
            std::string object = ParseKVRequest(kv_request);

            if (!first_batch_element) cur_batch_str.append(",");
            first_batch_element = false;
            cur_batch_str.append(object);
          }
        }
      }
      full_batches = cur_size == batch_size;
      cur_batch_str.append("]");

      if (cur_size > 0) values.append(cur_batch_str);

      min_seq += batch_size;
    }
    values.append("\n]");
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
      BatchClientRequest request;
      KVRequest kv_request;
      if (request.ParseFromString(txn.second)) {
        for (auto& sub_req : request.client_requests()) {
          kv_request.ParseFromString(sub_req.request().data());
          LOG(INFO) << "Block data:\n{\nseq: " << txn.first << "\n"
                    << kv_request.DebugString().c_str() << "}";
          std::string object = ParseKVRequest(kv_request);

          if (!first_iteration) values.append(",");

          first_iteration = false;

          values.append(object);
        }
      }
    }
    values.append("\n]");
    res.set_header("Content-Type", "application/json");
    res.end(values);
  });

  // Run the Crow app
  app.port(port_num_).multithreaded().run();
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

}  // namespace resdb
