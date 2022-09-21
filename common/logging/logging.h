#pragma once

#include <google/protobuf/message.h>

namespace resdb {

class Logging {
 public:
  Logging(const std::string& path);
  ~Logging();

  void Log(const google::protobuf::Message& message);
  void Log(const std::string& data);

  // For recovery.
  void ResetHead();
  void ResetEnd();
  int Read(google::protobuf::Message* info);

  // Only for test.
  void Clear();

 private:
  int fd_;
};
}  // namespace resdb
