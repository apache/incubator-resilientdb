#pragma once

#include <string>

#include "common/network/socket.h"

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
  virtual void ReInit();
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
