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
