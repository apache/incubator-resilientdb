#pragma once

#include <boost/asio.hpp>

#include "client/resdb_client.h"
#include "common/queue/lock_free_queue.h"
#include "proto/replica_info.pb.h"

namespace resdb {

class AsyncReplicaClient {
 public:
  AsyncReplicaClient(boost::asio::io_service* io_service, const std::string& ip,
                     int port, bool is_use_long_conn = false);
  virtual ~AsyncReplicaClient();

  virtual int SendMessage(const std::string& data);

 private:
  void ReConnect();
  void OnSendNewMessage();
  void OnSendMessage();
  void OnSend();

 private:
  LockFreeQueue<std::string> queue_;
  std::unique_ptr<ResDBClient> client_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::endpoint endpoint_;
  std::atomic<bool> in_process_;

  // ===== for async send =====
  std::unique_ptr<std::string> pending_data_;

  size_t data_size_ = 0;          // the size of the data needed to be sent.
  size_t sending_data_idx_ = 0;   // the current pos to be sent in data_ptr.
  size_t sending_data_size_ = 0;  // the size needed to be sent.
  const char* sending_data_ptr_ = nullptr;  // point to the data.

  int status_ = 0;  // sending status.
};

}  // namespace resdb
