#pragma once
#include <boost/asio.hpp>
#include <memory>

namespace resdb {

class AsyncAcceptor {
 public:
  typedef std::function<void(const char* buffer, size_t len)> CallBack;

  AsyncAcceptor(const std::string& ip, int thread_num, int port,
                CallBack call_back_func);
  virtual ~AsyncAcceptor();

  void StartAccept();

 private:
  class Session {
   public:
    Session(boost::asio::io_service* io_service, CallBack call_back_func);
    ~Session();

    boost::asio::ip::tcp::socket* GetSocket();

    void StartRead();
    void Close();

   private:
    void ReadDone();
    void OnRead();

   private:
    boost::asio::io_service* io_service_ = nullptr;
    boost::asio::ip::tcp::socket client_socket_;
    size_t data_size_ = 0;
    size_t need_size_ = 0;
    size_t current_idx_ = 0;
    char* recv_buffer_ = nullptr;
    bool status_ = 0;
    CallBack call_back_func_;
  };

 private:
  void OnAccept(boost::shared_ptr<Session> client_socket,
                const boost::system::error_code ec);

 private:
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::endpoint endpoint_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::unique_ptr<boost::asio::io_service::work> worker_;
  CallBack call_back_func_;
  std::vector<std::thread> worker_thread_;
  std::vector<boost::shared_ptr<Session>> sessions_;
};

}  // namespace resdb
