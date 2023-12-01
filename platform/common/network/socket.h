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
#include <string>

namespace resdb {

// Socket for Tcp Server
class Socket {
 public:
  Socket() = default;
  virtual ~Socket() = default;

  virtual int Connect(const std::string& ip, int port) = 0;
  virtual int Listen(const std::string& ip, int port) = 0;

  virtual void ReInit() = 0;
  virtual void Close() = 0;
  virtual std::unique_ptr<Socket> Accept() = 0;

  virtual int Send(const std::string& data) = 0;
  virtual int Recv(void** buf, size_t* len) = 0;

  virtual int SetSocketOpt(const char* option, int optval) { return -1; }
  virtual int GetBindingPort() = 0;

  virtual void SetRecvTimeout(int64_t microseconds) {}
  virtual void SetSendTimeout(int64_t microseconds) {}
  virtual int SetAsync(bool is_open = true) { return -1; }
};

}  // namespace resdb
