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

#include "client/resdb_user_client.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "common/network/mock_socket.h"
#include "common/test/test_macros.h"
#include "crypto/signature_verifier.h"
#include "proto/client_test.pb.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

std::string GenerateRequestData(const ClientTestRequest& client_request,
                                bool need_resp = false) {
  ResDBMessage message;

  Request request;
  request.set_type(Request::TYPE_CLIENT_REQUEST);
  request.set_need_response(need_resp);
  client_request.SerializeToString(request.mutable_data());
  request.SerializeToString(message.mutable_data());

  std::string request_data;
  message.SerializeToString(&request_data);

  return request_data;
}

std::string GenerateResponseData(const ClientTestResponse& client_responses) {
  std::string response_data;
  client_responses.SerializeToString(&response_data);
  return response_data;
}

class UserClientTest : public Test {
 public:
  UserClientTest() {
    self_info_.set_ip("127.0.0.1");
    self_info_.set_port(1234);

    dest_info_.set_ip("127.0.0.1");
    dest_info_.set_port(1235);
    replicas_.push_back(dest_info_);

    KeyInfo private_key;
    private_key.set_key("private_key");
    config_ = std::make_unique<ResDBConfig>(replicas_, self_info_, private_key,
                                            CertificateInfo());
    config_->SetClientTimeoutMs(10000);
  }

 protected:
  ReplicaInfo dest_info_;
  ReplicaInfo self_info_;
  std::vector<ReplicaInfo> replicas_;
  std::unique_ptr<ResDBConfig> config_;
};

TEST_F(UserClientTest, OnlySendRequestOK) {
  ClientTestRequest client_request;
  client_request.set_value("test_value");

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect(dest_info_.ip(), dest_info_.port()))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(GenerateRequestData(client_request)))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Recv(_, _)).Times(0);

  ResDBUserClient client(*config_);
  client.SetSocket(std::move(socket));
  client.SetSignatureVerifier(nullptr);

  int ret = client.SendRequest(client_request, Request::TYPE_CLIENT_REQUEST);
  EXPECT_EQ(ret, 0);
}

TEST_F(UserClientTest, GetResponse) {
  ClientTestRequest client_request;
  client_request.set_value("test_value");

  ClientTestResponse client_response, expected_response;
  expected_response.set_value("ack");

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect(dest_info_.ip(), dest_info_.port()))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(GenerateRequestData(client_request, true)))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Recv(_, _))
      .WillOnce(Invoke([&](void** buf, size_t* len) {
        ClientTestResponse response;
        response.set_value("ack");

        std::string resp_str = GenerateResponseData(response);

        *len = resp_str.size();
        *buf = malloc(*len);
        memcpy(*buf, resp_str.c_str(), *len);
        return *len;
      }));

  ResDBUserClient client(*config_);
  client.SetSocket(std::move(socket));
  EXPECT_EQ(client.SendRequest(client_request, &client_response,
                               Request::TYPE_CLIENT_REQUEST),
            0);
  EXPECT_THAT(client_response, EqualsProto(expected_response));
}

TEST_F(UserClientTest, RecvResponseFail) {
  ClientTestRequest client_request;
  client_request.set_value("test_value");

  ClientTestResponse client_response;

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect(dest_info_.ip(), dest_info_.port()))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(GenerateRequestData(client_request, true)))
      .WillOnce(Return(0));
  EXPECT_CALL(*socket, Recv(_, _)).WillOnce(Return(-1));

  ResDBUserClient client(*config_);
  client.SetSocket(std::move(socket));

  EXPECT_NE(client.SendRequest(client_request, &client_response,
                               Request::TYPE_CLIENT_REQUEST),
            0);
}

}  // namespace

}  // namespace resdb
