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
      .methods("POST"_method)([this](const crow::request& req) {
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
  ([this](crow::response& res, std::string id) {
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

}  // namespace

}  // namespace resdb
