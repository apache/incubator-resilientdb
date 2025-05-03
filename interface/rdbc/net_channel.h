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

#pragma once
#include <memory>

#include "common/crypto/signature_verifier.h"
#include "platform/common/network/socket.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {

// NetChannel is used to send data to the server identified by ip:port or
// via the provided socket. If SignatureVerifier is provided, data will be
// signed. The client will retry max_retry_time_ if the connection or sned
// process fail. For a message with a command, it will be set inside a Request
// message. Each Request will be put into the network message ResDBMessage and
// signed.
//
// Message structre:
// ResDBMessage:
//     Request:
//        cmd
//        user_message
//     signature
//
// For a raw message, it will be set inside a Network Message directly.
// Message structre:
// ResDBMessage:
//     user_message
//     signature
//
class NetChannel {
 public:
  NetChannel(const std::string& ip, int port);
  // Use the provided socket to send data. If the socket has been connect to
  // the server, it won't connect again.
  NetChannel(std::unique_ptr<Socket> socket, bool connected = false);

  virtual ~NetChannel() = default;

  void Close();

  void SetSocket(std::unique_ptr<Socket> socket);
  void SetSignatureVerifier(SignatureVerifier* verifier);
  void SetDestReplicaInfo(const ReplicaInfo& replica);

  // Send a message request to the server with a commend.
  // A new request will be generated with command cmd and contain the message.
  virtual int SendRequest(const google::protobuf::Message& message,
                          Request::Type cmd, bool need_response = false);

  // Send the message to the server directly.
  virtual int SendRawMessage(const google::protobuf::Message& message);
  virtual int SendRawMessageData(const std::string& message_str);

  // Recv the data inside ResDBMessage.
  virtual int RecvRawMessageStr(std::string* message);
  virtual int RecvRawMessage(google::protobuf::Message* message);

  // Recv the raw string from the server.
  virtual int RecvRawMessageData(std::string* message_str);

  static std::string GetRawMessageString(
      const google::protobuf::Message& message,
      SignatureVerifier* verifier = nullptr);

  void SetRecvTimeout(int microseconds);
  void IsLongConnection(bool long_connect_tion);
  void SetAsyncSend(bool is_async_send);

 protected:
  int SendDataInternal(const std::string& data);
  int SendFromKeepAlive(const std::string& data);
  int Send(const std::string& data);
  int Recv(std::string* data);
  int Connect();

 protected:
  SignatureVerifier* verifier_ = nullptr;

  std::unique_ptr<Socket> socket_;
  int max_retry_time_ = 3;
  std::string ip_;
  int port_;
  bool connected_ = false;
  int read_timeouts_ = 1000000;  // timeout for 1s.
  bool long_connect_tion_ = false;
  bool long_connecting_ = false;
  bool is_async_send_ = false;
};

}  // namespace resdb
