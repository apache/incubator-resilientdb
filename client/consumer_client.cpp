#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

using namespace std;
using namespace AmqpClient;

void consumeMessage() {
  Channel::OpenOpts openOpts = Channel::OpenOpts();
  openOpts.host = "localhost";
  openOpts.port = 5673;
  Channel::OpenOpts::BasicAuth basicAuth = Channel::OpenOpts::BasicAuth("guest","guest");
  openOpts.auth = basicAuth;
  Channel::ptr_t connection = Channel::Open(openOpts);
  std::string consumer_tag = connection->BasicConsume("custom_queue", "");
  Envelope::ptr_t envelope = connection->BasicConsumeMessage(consumer_tag);
  cout << envelope->Message()->Body();
  connection->BasicConsumeMessage(consumer_tag, envelope, 10);  // 10 ms timeout
}

void publishMessage() {
  Channel::OpenOpts openOpts = Channel::OpenOpts();
  openOpts.host = "localhost";
  openOpts.port = 5673;
  Channel::OpenOpts::BasicAuth basicAuth = Channel::OpenOpts::BasicAuth("guest","guest");
  openOpts.auth = basicAuth;
  Channel::ptr_t connection = Channel::Open(openOpts);
  string producerMessage = "this is a test message 2";
  connection->BasicPublish("TestExchange", "TQ1",
                           BasicMessage::Create(producerMessage));
}

class CallBackTimer {
 public:
  CallBackTimer() : _execute(false) {}

  ~CallBackTimer() {
    if (_execute.load(std::memory_order_acquire)) {
      stop();
    };
  }

  void stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) _thd.join();
  }

  void start(int interval, std::function<void(void)> func) {
    if (_execute.load(std::memory_order_acquire)) {
      stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, interval, func]() {
      while (_execute.load(std::memory_order_acquire)) {
        func();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
      }
    });
  }

  bool is_running() const noexcept {
    return (_execute.load(std::memory_order_acquire) && _thd.joinable());
  }

 private:
  std::atomic<bool> _execute;
  std::thread _thd;
};

class Processor {
  CallBackTimer timer;

 public:
  void init() { timer.start(1000, std::bind(&Processor::process, this)); }

  void process() {
    Channel::OpenOpts openOpts = Channel::OpenOpts();
    openOpts.host = "localhost";
    openOpts.port = 5673;
    Channel::OpenOpts::BasicAuth basicAuth = Channel::OpenOpts::BasicAuth("guest","guest");
    openOpts.auth = basicAuth;
    Channel::ptr_t connection = Channel::Open(openOpts);
    std::string consumer_tag = connection->BasicConsume("custom_queue", "");
    Envelope::ptr_t envelope = connection->BasicConsumeMessage(consumer_tag);
    cout << envelope->Message()->Body() << endl;
    connection->BasicConsumeMessage(consumer_tag, envelope,
                                    1);  // 10 ms timeout
  }
};

int main() {
  Processor proc;
  proc.init();
  while (true)
    ;
}