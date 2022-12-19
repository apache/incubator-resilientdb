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

#include "client/resdb_client.h"

#include <gtest/gtest.h>

#include "common/network/mock_socket.h"
#include "crypto/key_generator.h"
#include "proto/client_test.pb.h"

namespace resdb {
namespace {

using ::testing::Invoke;
using ::testing::Return;
using ::testing::Test;

class ResDBClientTest : public Test {
 protected:
  ResDBMessage GetSendPackage(const google::protobuf::Message& message,
                              Request::Type type) {
    ResDBMessage resdb_message;
    Request request;
    request.set_type(type);
    EXPECT_TRUE(message.SerializeToString(request.mutable_data()));
    EXPECT_TRUE(request.SerializeToString(resdb_message.mutable_data()));
    return resdb_message;
  }

  std::string GetSendPackageString(const google::protobuf::Message& message,
                                   Request::Type type) {
    ResDBMessage resdb_message = GetSendPackage(message, type);
    std::string data;
    EXPECT_TRUE(resdb_message.SerializeToString(&data));
    return data;
  }

  ResDBMessage GetSendPackage(const google::protobuf::Message& message) {
    ResDBMessage resdb_message;
    EXPECT_TRUE(message.SerializeToString(resdb_message.mutable_data()));
    return resdb_message;
  }

  std::string GetSendPackageString(const google::protobuf::Message& message) {
    ResDBMessage resdb_message = GetSendPackage(message);
    std::string data;
    EXPECT_TRUE(resdb_message.SerializeToString(&data));
    return data;
  }
};

TEST_F(ResDBClientTest, ClientConnectFail) {
  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234))
      .Times(3)
      .WillRepeatedly(Return(-1));
  ResDBClient client("127.0.0.1", 1234);

  client.SetSocket(std::move(socket));

  ClientTestRequest client_request;
  std::string data;
  ASSERT_TRUE(client_request.SerializeToString(&data));
  int ret = client.SendRawMessage(client_request);
  EXPECT_LE(ret, 0);
}

TEST_F(ResDBClientTest, KeepAliveAndAsyncReConnect) {
  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234))
      .Times(3)
      .WillRepeatedly(Return(-1));

  EXPECT_CALL(*socket, SetAsync(true)).Times(3).WillRepeatedly(Return(0));

  ResDBClient client("127.0.0.1", 1234);
  client.SetAsyncSend(true);
  client.IsLongConnection(true);

  client.SetSocket(std::move(socket));

  ClientTestRequest client_request;
  std::string data;
  ASSERT_TRUE(client_request.SerializeToString(&data));
  int ret = client.SendRawMessage(client_request);
  EXPECT_LE(ret, 0);
}

TEST_F(ResDBClientTest, SendRequest) {
  std::string data = "test_data";

  ClientTestRequest client_request;
  client_request.set_value("test_value");

  std::string expected_request_str =
      GetSendPackageString(client_request, Request::TYPE_CLIENT_REQUEST);

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234)).WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(expected_request_str)).WillOnce(Return(0));

  ResDBClient client("127.0.0.1", 1234);
  client.SetSocket(std::move(socket));
  client.SetSignatureVerifier(nullptr);

  int ret = client.SendRequest(client_request, Request::TYPE_CLIENT_REQUEST);
  EXPECT_EQ(ret, 0);
}

TEST_F(ResDBClientTest, SendRequestFail) {
  std::string data = "test_data";

  ClientTestRequest client_request;
  client_request.set_value("test_value");

  std::string expected_request_str =
      GetSendPackageString(client_request, Request::TYPE_CLIENT_REQUEST);

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, ReInit()).Times(3);
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234)).Times(3).WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(expected_request_str))
      .Times(3)
      .WillRepeatedly(Return(-1));

  ResDBClient client("127.0.0.1", 1234);
  client.SetSocket(std::move(socket));
  client.SetSignatureVerifier(nullptr);

  int ret = client.SendRequest(client_request, Request::TYPE_CLIENT_REQUEST);
  EXPECT_NE(ret, 0);
}

TEST_F(ResDBClientTest, SendMessage) {
  ClientTestRequest client_request;
  client_request.set_value("test_value");

  std::string data = GetSendPackageString(client_request);

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234)).WillOnce(Return(0));
  EXPECT_CALL(*socket, Send(data)).WillOnce(Return(0));

  ResDBClient client("127.0.0.1", 1234);
  client.SetSocket(std::move(socket));
  client.SetSignatureVerifier(nullptr);

  EXPECT_EQ(client.SendRawMessage(client_request), 0);
}

TEST_F(ResDBClientTest, SignMessage) {
  SecretKey my_key = KeyGenerator ::GeneratorKeys(SignatureInfo::RSA);
  int64_t my_node_id = 1;

  KeyInfo private_key;
  private_key.set_key(my_key.private_key());
  private_key.set_hash_type(my_key.hash_type());
  CertificateInfo cert_info;
  cert_info.set_node_id(my_node_id);
  cert_info.mutable_public_key()
      ->mutable_public_key_info()
      ->mutable_key()
      ->set_key(my_key.public_key());
  cert_info.mutable_public_key()
      ->mutable_public_key_info()
      ->mutable_key()
      ->set_hash_type(my_key.hash_type());
  cert_info.mutable_public_key()->mutable_public_key_info()->set_node_id(
      my_node_id);

  SignatureVerifier verifier(private_key, cert_info);

  ClientTestRequest client_request;
  client_request.set_value("test_value");

  std::string data = GetSendPackageString(client_request);

  std::unique_ptr<MockSocket> socket = std::make_unique<MockSocket>();
  EXPECT_CALL(*socket, Connect("127.0.0.1", 1234)).WillOnce(Return(0));
  EXPECT_CALL(*socket, Send).WillOnce(Invoke([&](const std::string& data) {
    ResDBMessage resdb_message;
    EXPECT_TRUE(resdb_message.ParseFromString(data));
    EXPECT_TRUE(verifier.VerifyMessage(resdb_message.data(),
                                       resdb_message.signature()));
    return 0;
  }));

  ResDBClient client("127.0.0.1", 1234);
  client.SetSocket(std::move(socket));
  client.SetSignatureVerifier(&verifier);

  EXPECT_EQ(client.SendRawMessage(client_request), 0);
}

}  // namespace

}  // namespace resdb
