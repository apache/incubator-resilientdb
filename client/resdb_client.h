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

#pragma once
#include <memory>

#include "common/network/socket.h"
#include "crypto/signature_verifier.h"
#include "proto/resdb.pb.h"

namespace resdb {

// ResDBClient is used to send data to the server identified by ip:port or via
// the provided socket. If SignatureVerifier is provided, data will be signed.
// The client will retry max_retry_time_ if the connection or sned process fail.
// For a message with a command, it will be set inside a Request message.
// Each Request will be put into the network message ResDBMessage and signed.
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
class ResDBClient {
 public:
  ResDBClient(const std::string& ip, int port);
  // Use the provided socket to send data. If the socket has been connect to
  // the server, it won't connect again.
  ResDBClient(std::unique_ptr<Socket> socket, bool connected = false);

  virtual ~ResDBClient() = default;

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
