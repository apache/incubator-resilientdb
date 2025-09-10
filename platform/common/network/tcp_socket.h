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

#include <string>

#include "platform/common/network/socket.h"

namespace resdb {

// Socket for Tcp Server
class TcpSocket : public Socket {
 public:
  TcpSocket();
  TcpSocket(int socket_fd);
  ~TcpSocket();

  // For Server
  int Listen(const std::string& ip, int port) override;
  std::unique_ptr<Socket> Accept() override;
  void ReInit() override;
  void Close() override;

  int Connect(const std::string& ip, int port) override;

  int Send(const std::string& data) override;
  int Recv(void** buf, size_t* len) override;

  int GetBindingPort() override;

  void SetRecvTimeout(int64_t microseconds) override;
  void SetSendTimeout(int64_t microseconds) override;
  int SetAsync(bool is_open = true) override;

 private:
  int InitSocket();

 private:
  int socket_fd_;
  int binding_port_ = 0;
};
}  // namespace resdb
