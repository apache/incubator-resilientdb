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

CrowService::CrowService(ResDBConfig config, uint16_t port_num)
    : config_(config), port_num_(port_num) {}

void CrowService::run() {
  crow::SimpleApp app;
  CROW_ROUTE(app, "/")
  ([]() { return "Welcome to the home page"; });

  CROW_ROUTE(app, "/v1/transactions")
  ([this](const crow::request& req, response& res) {
    ResDBKVClient client(config_);
    auto values = client.GetValues();
    if (values != nullptr) {
      LOG(INFO) << "client getvalues value = " << values->c_str();
    }
    res.set_header("Content-Type", "application/json");
    res.end(std::string(values->c_str()));
  });

  CROW_ROUTE(app, "/v1/transactions/<string>")
  ([this](const crow::request& req, response& res, std::string id) {
    ResDBKVClient client(config_);
    auto value = client.Get(id);
    if (value != nullptr) {
      LOG(INFO) << "client get value = " << value->c_str();
      res.set_header("Content-Type", "application/json");
      res.end(std::string(value->c_str()));
    } else {
      res.set_header("Content-Type", "text/plain");
      res.end("get value fail");
    }
  });

  CROW_ROUTE(app, "/v1/transactions/commit")
      .methods("POST"_method)([this](const request& req) {
        std::string body = req.body;
        resdb::SDKTransaction transaction = resdb::fromJson(body);
        const std::string id = transaction.id;
        const std::string value = transaction.value;

        // Set key-value pair in kv server
        ResDBKVClient client(config_);
        int retval = client.Set(id, value);

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

  // set the port, set the app to run on multiple threads, and run the app
  app.port(port_num_).multithreaded().run();
}

}  // namespace resdb
