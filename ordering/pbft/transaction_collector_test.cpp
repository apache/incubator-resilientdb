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

#include "ordering/pbft/transaction_collector.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;

TEST(TransactionCollectorTest, SeqError) {
  int64_t seq = 11111;
  TransactionCollector collector(seq, nullptr);

  std::unique_ptr<Request> request = std::make_unique<Request>();
  SignatureInfo signature;

  EXPECT_EQ(
      collector.AddRequest(std::move(request), signature,
                           /* is_main_request =*/true,
                           [&](const Request& request, int received_count,
                               TransactionCollector::CollectorDataType* data,
                               std::atomic<TransactionStatue>* status) {}),
      -2);
}

TEST(TransactionCollectorTest, AddContextList) {
  std::vector<std::unique_ptr<Context>> context_list;

  for (int i = 0; i < 1000000; ++i) {
    auto context = std::make_unique<Context>();
    context->signature = SignatureInfo();

    context_list.push_back(std::move(context));
  }
  TransactionCollector collector(1, nullptr);
  EXPECT_EQ(collector.SetContextList(1, std::move(context_list)), 0);
}

TEST(TransactionCollectorTest, AddMainRequest) {
  int64_t seq = 11111;
  TransactionCollector collector(seq, nullptr);

  Request expect_request;
  expect_request.set_seq(seq);
  std::unique_ptr<Request> request = std::make_unique<Request>(expect_request);
  SignatureInfo signature;
  bool is_called = false;
  EXPECT_EQ(
      collector.AddRequest(std::move(request), signature,
                           /* is_main_request =*/true,
                           [&](const Request& request, int received_count,
                               TransactionCollector::CollectorDataType* data,
                               std::atomic<TransactionStatue>* status) {
                             EXPECT_THAT(request, EqualsProto(expect_request));
                             EXPECT_EQ(received_count, 1);
                             EXPECT_EQ(*status, TransactionStatue::None);
                             is_called = true;
                           }),
      0);
  EXPECT_TRUE(is_called);
}

TEST(TransactionCollectorTest, AddSameMainRequest) {
  int64_t seq = 11111;
  TransactionCollector collector(seq, nullptr);

  Request expect_request;
  expect_request.set_seq(seq);
  expect_request.set_hash("hash_main");

  {
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/true,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  {
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/true,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              -2);
    EXPECT_FALSE(is_called);
  }
}

TEST(TransactionCollectorTest, AddTypeRequest) {
  int64_t seq = 11111;
  TransactionCollector collector(seq, nullptr);

  Request expect_request;
  expect_request.set_seq(seq);
  expect_request.set_hash("hash_main");

  // Add main request
  {
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/true,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add 1 pre-prepare message.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add 1 pre-prepare message with the same sender, should return received
  // count 1.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add 1 pre-prepare message with different sender, should return received
  // count 2. Set the statue to prepare.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    expect_request.set_sender_id(1);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 2);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    *status = TransactionStatue::Prepare;
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add Prepare message.
  {
    expect_request.set_type(Request::TYPE_PREPARE);
    expect_request.set_sender_id(1);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::Prepare);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add old type message, status should not change.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    expect_request.set_sender_id(3);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 3);
                    EXPECT_EQ(*status, TransactionStatue::Prepare);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
}

TEST(TransactionCollectorTest, DelayMainRequest) {
  int64_t seq = 11111;
  TransactionCollector collector(seq, nullptr);

  Request expect_request;
  expect_request.set_seq(seq);
  expect_request.set_hash("hash_main");

  // Add 1 pre-prepare message.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add main request
  {
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/true,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }

  // Add 1 pre-prepare message with different sender, should return received
  // count 2. Set the statue to prepare.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    expect_request.set_sender_id(1);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 2);
                    EXPECT_EQ(*status, TransactionStatue::None);
                    *status = TransactionStatue::Prepare;
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add Prepare message.
  {
    expect_request.set_type(Request::TYPE_PREPARE);
    expect_request.set_sender_id(1);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 1);
                    EXPECT_EQ(*status, TransactionStatue::Prepare);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
  // Add old type message, status should not change.
  {
    expect_request.set_type(Request::TYPE_PRE_PREPARE);
    expect_request.set_sender_id(3);
    std::unique_ptr<Request> request =
        std::make_unique<Request>(expect_request);
    SignatureInfo signature;
    bool is_called = false;
    EXPECT_EQ(collector.AddRequest(
                  std::move(request), signature,
                  /* is_main_request =*/false,
                  [&](const Request& request, int received_count,
                      TransactionCollector::CollectorDataType* data,
                      std::atomic<TransactionStatue>* status) {
                    EXPECT_THAT(request, EqualsProto(expect_request));
                    EXPECT_EQ(received_count, 3);
                    EXPECT_EQ(*status, TransactionStatue::Prepare);
                    is_called = true;
                  }),
              0);
    EXPECT_TRUE(is_called);
  }
}

}  // namespace

}  // namespace resdb
