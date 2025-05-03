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

#include "platform/consensus/ordering/pbft/response_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "interface/rdbc/mock_net_channel.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/networkstrate/mock_replica_communicator.h"

namespace resdb {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Test;

CertificateInfo GetCertificateInfo() {
  CertificateInfo info;
  info.mutable_public_key()->mutable_public_key_info()->set_type(
      CertificateKeyInfo::CLIENT);
  return info;
}

class ResponseManagerTest : public Test {
 public:
  ResponseManagerTest()
      :  // just set the monitor time to 1 second to return early.
        global_stats_(Stats::GetGlobalStats(1)),
        config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(1, "127.0.0.1", 1234), KeyInfo(),
                GetCertificateInfo()),
        system_info_(config_),
        manager_(config_, &replica_communicator_, &system_info_, nullptr) {
    global_stats_->Stop();
  }

  int AddResponseMsg(int sender_id, BatchUserResponse batch_resp) {
    Request request;
    request.set_current_view(1);
    request.set_seq(1);
    request.set_type(Request::TYPE_RESPONSE);
    request.set_sender_id(sender_id);
    request.set_data("resp_data");
    batch_resp.SerializeToString(request.mutable_data());

    return manager_.ProcessResponseMsg(std::make_unique<Context>(),
                                       std::make_unique<Request>(request));
  }

 protected:
  Stats* global_stats_;
  ResDBConfig config_;
  SystemInfo system_info_;
  MockReplicaCommunicator replica_communicator_;
  ResponseManager manager_;
};

TEST_F(ResponseManagerTest, GetConext) {
  Request request;
  request.set_seq(1);

  std::vector<std::unique_ptr<Context>> list(2);

  EXPECT_EQ(manager_.AddContextList(std::move(list), 1), 0);
  EXPECT_TRUE(manager_.FetchContextList(2).empty());

  EXPECT_FALSE(manager_.FetchContextList(1).empty());
  EXPECT_TRUE(manager_.FetchContextList(1).empty());
}

TEST_F(ResponseManagerTest, SendUserRequest) {
  auto context = std::make_unique<Context>();
  context->signature.set_signature("signature");

  std::promise<bool> propose_done;
  std::future<bool> propose_done_future = propose_done.get_future();
  EXPECT_CALL(replica_communicator_, SendMessage(_, 1)).WillOnce(Invoke([&]() {
    propose_done.set_value(true);
  }));

  EXPECT_EQ(
      manager_.NewUserRequest(std::move(context), std::make_unique<Request>()),
      0);
  propose_done_future.get();
}

TEST_F(ResponseManagerTest, ProcessResponse) {
  std::unique_ptr<MockNetChannel> channel =
      std::make_unique<MockNetChannel>("127.0.0.1", 0);

  EXPECT_CALL(*channel, SendRawMessageData).Times(1);

  uint64_t local_id = 1;

  BatchUserResponse batch_resp;
  *batch_resp.add_response() = "test";
  *batch_resp.add_signatures() = SignatureInfo();
  batch_resp.set_local_id(local_id);

  Request request;
  request.set_sender_id(1);
  request.set_current_view(1);
  request.set_seq(1);
  request.set_proxy_id(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(channel);
  context->signature.set_signature("signature");

  std::vector<std::unique_ptr<Context>> context_list;
  context_list.push_back(std::move(context));
  manager_.AddContextList(std::move(context_list), local_id);

  EXPECT_EQ(AddResponseMsg(1, batch_resp), 0);
  EXPECT_EQ(AddResponseMsg(2, batch_resp), 0);
}

TEST_F(ResponseManagerTest, ProcessResponseWithMoreResp) {
  std::unique_ptr<MockNetChannel> channel =
      std::make_unique<MockNetChannel>("127.0.0.1", 0);

  EXPECT_CALL(*channel, SendRawMessageData).Times(1);

  uint64_t local_id = 1;

  BatchUserResponse batch_resp;
  *batch_resp.add_response() = "test";
  *batch_resp.add_signatures() = SignatureInfo();
  batch_resp.set_local_id(local_id);

  Request request;
  request.set_sender_id(1);
  request.set_current_view(1);
  request.set_seq(1);
  request.set_proxy_id(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(channel);
  context->signature.set_signature("signature");

  std::vector<std::unique_ptr<Context>> context_list;
  context_list.push_back(std::move(context));
  manager_.AddContextList(std::move(context_list), local_id);

  EXPECT_EQ(AddResponseMsg(1, batch_resp), 0);
  EXPECT_EQ(AddResponseMsg(2, batch_resp), 0);
  // do nothing.
  EXPECT_EQ(AddResponseMsg(3, batch_resp), -2);
}

TEST_F(ResponseManagerTest, ProcessResponseWithSameSender) {
  std::unique_ptr<MockNetChannel> channel =
      std::make_unique<MockNetChannel>("127.0.0.1", 0);

  EXPECT_CALL(*channel, SendRawMessageData).Times(0);

  uint64_t local_id = 1;

  BatchUserResponse batch_resp;
  *batch_resp.add_response() = "test";
  *batch_resp.add_signatures() = SignatureInfo();
  batch_resp.set_local_id(local_id);

  Request request;
  request.set_sender_id(1);
  request.set_current_view(1);
  request.set_seq(1);
  request.set_proxy_id(1);

  auto context = std::make_unique<Context>();
  context->client = std::move(channel);
  context->signature.set_signature("signature");

  std::vector<std::unique_ptr<Context>> context_list;
  context_list.push_back(std::move(context));
  manager_.AddContextList(std::move(context_list), local_id);

  EXPECT_EQ(AddResponseMsg(1, batch_resp), 0);
  EXPECT_EQ(AddResponseMsg(1, batch_resp), 0);
}

}  // namespace

}  // namespace resdb
