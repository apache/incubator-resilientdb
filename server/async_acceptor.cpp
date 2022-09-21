#include "server/async_acceptor.h"

#include <glog/logging.h>
#include <signal.h>

#include <thread>

namespace resdb {

AsyncAcceptor::Session::Session(boost::asio::io_service* io_service,
                                CallBack call_back_func)
    : io_service_(io_service),
      client_socket_(*io_service_),
      recv_buffer_(nullptr),
      status_(0),
      call_back_func_(call_back_func) {}

AsyncAcceptor::Session::~Session() { Close(); }

boost::asio::ip::tcp::socket* AsyncAcceptor::Session::GetSocket() {
  return &client_socket_;
}

void AsyncAcceptor::Session::Close() {
  if (client_socket_.is_open()) {
    client_socket_.cancel();
  }
  if (recv_buffer_ && status_ != 0) {
    delete recv_buffer_;
    recv_buffer_ = nullptr;
  }
}

void AsyncAcceptor::Session::StartRead() {
  if (status_ == 0) {
    // read len
    recv_buffer_ = reinterpret_cast<char*>(&data_size_);
    need_size_ = sizeof(data_size_);
    current_idx_ = 0;
    OnRead();
  } else {
    need_size_ = data_size_;
    current_idx_ = 0;
    if (recv_buffer_) {
      delete recv_buffer_;
    }
    recv_buffer_ = new char[need_size_];
    OnRead();
  }
}

void AsyncAcceptor::Session::ReadDone() {
  if (status_ == 1) {
    call_back_func_(recv_buffer_, data_size_);
    delete recv_buffer_;
  } else {
    data_size_ = *reinterpret_cast<size_t*>(recv_buffer_);
  }
  status_ ^= 1;
  recv_buffer_ = nullptr;
  // continue to read next msg.
  StartRead();
}

void AsyncAcceptor::Session::OnRead() {
  client_socket_.async_read_some(
      boost::asio::buffer(recv_buffer_ + current_idx_,
                          need_size_ - current_idx_),
      [&](const boost::system::error_code& error,  // Result of operation.
          std::size_t bytes_transferred) {
        if (error || bytes_transferred == 0) {
          Close();
        } else {
          current_idx_ += bytes_transferred;
          if (current_idx_ >= need_size_) {
            ReadDone();
          } else {
            OnRead();
          }
        }
      });
}

AsyncAcceptor::AsyncAcceptor(const std::string& ip, int port, int thread_num,
                             CallBack call_back_func)
    : endpoint_(boost::asio::ip::address::from_string(ip), port),
      acceptor_(io_service_, endpoint_),
      call_back_func_(call_back_func) {
  worker_ = std::make_unique<boost::asio::io_service::work>(io_service_);
  for (int i = 0; i < thread_num; ++i) {
    worker_thread_.push_back(std::thread([&]() { io_service_.run(); }));
  }
}

AsyncAcceptor::~AsyncAcceptor() {
  worker_.reset();
  worker_ = nullptr;
  io_service_.stop();

  for (auto& sess : sessions_) {
    if (sess) {
      sess->Close();
    }
  }

  for (auto& worker : worker_thread_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void AsyncAcceptor::StartAccept() {
  boost::shared_ptr<Session> client_session(
      new Session(&io_service_, call_back_func_));
  acceptor_.async_accept(*client_session->GetSocket(),
                         std::bind(&AsyncAcceptor::OnAccept, this,
                                   client_session, std::placeholders::_1));
}

void AsyncAcceptor::OnAccept(boost::shared_ptr<Session> client_session,
                             const boost::system::error_code ec) {
  if (ec) {
    LOG(ERROR) << " accept fail";
    return;
  }

  StartAccept();  // Add the next accept event.

  sessions_.push_back(client_session);
  client_session->StartRead();
}

}  // namespace resdb
