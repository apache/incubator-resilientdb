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

#include "platform/consensus/ordering/pbft/response_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "platform/communication/mock_replica_communicator.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/interface/mock_resdb_net_channel.h"

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
  std::unique_ptr<MockResDBNetChannel> channel =
      std::make_unique<MockResDBNetChannel>("127.0.0.1", 0);

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
  std::unique_ptr<MockResDBNetChannel> channel =
      std::make_unique<MockResDBNetChannel>("127.0.0.1", 0);

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
  std::unique_ptr<MockResDBNetChannel> channel =
      std::make_unique<MockResDBNetChannel>("127.0.0.1", 0);

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
