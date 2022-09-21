#include "common/logging/logging.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace resdb {

// TODO write to files
Logging::Logging(const std::string &path) {
  fd_ = open(path.c_str(), O_CREAT | O_APPEND | O_RDWR, 0666);
  assert(fd_ >= 0);
}

Logging::~Logging() { close(fd_); }

void Logging::Clear() { ftruncate(fd_, 0); }

void Logging::Log(const google::protobuf::Message &message) {
  std::string str;
  if (message.SerializeToString(&str)) {
    Log(str);
  }
}

void Logging::Log(const std::string &data) {
  size_t size = data.size();
  write(fd_, &size, sizeof(size));
  write(fd_, data.c_str(), data.size());
}

void Logging::ResetHead() { lseek(fd_, 0, SEEK_SET); }

void Logging::ResetEnd() { lseek(fd_, 0, SEEK_END); }

int Logging::Read(google::protobuf::Message *info) {
  size_t size;
  if (read(fd_, &size, sizeof(size)) != sizeof(size)) {
    LOG(ERROR) << "read len fail:" << strerror(errno);
    return -1;
  }
  if (size == 0) {
    return 0;
  }
  char *buffer = new char[size];
  size_t ret = read(fd_, buffer, size);
  assert(ret == size);
  assert(info->ParseFromArray(buffer, size));
  free(buffer);
  return 0;
}

}  // namespace resdb
