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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace resdb {
namespace {

using ::testing::Test;

class CrowServiceTest : public Test {
protected:
  crow::SimpleApp app;
};

// Test passing string through POST method
TEST_F(CrowServiceTest, CommitTransaction) {
  std::string str;

  CROW_ROUTE(app, "/v1/transactions/commit")
      .methods("POST"_method)([this](const crow::request &req) {
        crow::response res(201, req.body);
        res.set_header("Content-Type", "text/plain");
        return res;
      });

  app.validate();
  app.debug_print();
  crow::request req;
  req.body = "body";
  req.method = "POST"_method;
  req.url = "/v1/transactions/commit";

  crow::response res;
  app.handle(req, res);

  EXPECT_EQ(res.body, "body");
}

// Test passing string parameter through URL
TEST_F(CrowServiceTest, GetTransaction) {
  CROW_ROUTE(app, "/v1/transactions/<string>")
  ([this](crow::response &res, std::string id) {
    res.set_header("Content-Type", "text/plain");
    res.end(id);
  });
  app.validate();
  app.debug_print();
  crow::request req;
  crow::response res;
  req.url = "/v1/transactions/test_str";
  app.handle(req, res);
  EXPECT_EQ(res.body, "test_str");
}

// Test passing two int parameters through URL
TEST_F(CrowServiceTest, GetBlockRange) {
  int A, B;
  CROW_ROUTE(app, "/v1/blocks/<int>/<int>")
  ([&](int a, int b) {
    A = a;
    B = b;
    return "OK";
  });
  app.validate();
  app.debug_print();
  crow::request req;
  crow::response res;
  req.url = "/v1/blocks/1/2";
  app.handle(req, res);
  EXPECT_EQ(A, 1);
  EXPECT_EQ(B, 2);
}

} // namespace

} // namespace resdb
