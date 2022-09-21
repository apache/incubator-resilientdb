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
