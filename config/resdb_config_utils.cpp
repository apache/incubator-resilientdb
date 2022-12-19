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

#include "config/resdb_config_utils.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <google/protobuf/util/json_util.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>

namespace resdb {

using ::google::protobuf::util::JsonParseOptions;

namespace {

KeyInfo ReadKey(const std::string& file_name) {
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd < 0) {
    LOG(ERROR) << "open file:" << file_name << " fail:" << strerror(errno);
  }
  assert(fd >= 0);

  std::string res;
  int read_len = 0;
  char tmp[1024];
  while (true) {
    read_len = read(fd, tmp, sizeof(tmp));
    if (read_len <= 0) {
      break;
    }
    res.append(tmp, read_len);
  }
  close(fd);
  KeyInfo key;
  assert(key.ParseFromString(res));
  return key;
}

CertificateInfo ReadCert(const std::string& file_name) {
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd < 0) {
    LOG(ERROR) << "open file:" << file_name << " fail" << strerror(errno);
  }
  assert(fd >= 0);

  std::string res;
  int read_len = 0;
  char tmp[1024];
  while (true) {
    read_len = read(fd, tmp, sizeof(tmp));
    if (read_len <= 0) {
      break;
    }
    res.append(tmp, read_len);
  }
  close(fd);
  CertificateInfo info;
  assert(info.ParseFromString(res));
  return info;
}

}  // namespace

ReplicaInfo GenerateReplicaInfo(int id, const std::string& ip, int port) {
  ReplicaInfo info;
  info.set_id(id);
  info.set_ip(ip);
  info.set_port(port);
  return info;
}

ResConfigData ReadConfigFromFile(const std::string& file_name) {
  std::stringstream json_data;
  std::ifstream infile(file_name.c_str());
  json_data << infile.rdbuf();

  ResConfigData config_data;
  JsonParseOptions options;
  auto status = JsonStringToMessage(json_data.str(), &config_data, options);
  if (!status.ok()) {
    LOG(ERROR) << "parse json :" << file_name << " fail:" << status.message();
  }
  assert(status.ok());
  return config_data;
}

std::vector<ReplicaInfo> ReadConfig(const std::string& file_name) {
  std::vector<ReplicaInfo> replicas;
  std::string line;
  std::ifstream infile(file_name.c_str());
  int id;
  std::string ip;
  int port;
  while (infile >> id >> ip >> port) {
    replicas.push_back(GenerateReplicaInfo(id, ip, port));
  }
  if (replicas.size() == 0) {
    LOG(ERROR) << "read config:" << file_name << " fail.";
    assert(replicas.size() > 0);
  }
  return replicas;
}

std::unique_ptr<ResDBConfig> GenerateResDBConfig(
    const std::string& config_file, const std::string& private_key_file,
    const std::string& cert_file, std::optional<ReplicaInfo> self_info,
    std::optional<ConfigGenFunc> gen_func) {
  ResConfigData config_data = ReadConfigFromFile(config_file);
  KeyInfo private_key = ReadKey(private_key_file);
  CertificateInfo cert_info = ReadCert(cert_file);

  LOG(ERROR) << "private key:" << private_key.DebugString()
             << " cert:" << cert_info.DebugString();
  if (!self_info.has_value()) {
    self_info = ReplicaInfo();
  }

  (*self_info).set_id(cert_info.public_key().public_key_info().node_id());
  (*self_info).set_ip(cert_info.public_key().public_key_info().ip());
  (*self_info).set_port(cert_info.public_key().public_key_info().port());

  *(*self_info).mutable_certificate_info() = cert_info;

  if (gen_func.has_value()) {
    return (*gen_func)(config_data, self_info.value(), private_key, cert_info);
  }
  return std::make_unique<ResDBConfig>(config_data, self_info.value(),
                                       private_key, cert_info);
}

ResDBConfig GenerateResDBConfig(const std::string& config_file) {
  std::vector<ReplicaInfo> replicas = ReadConfig(config_file);
  return ResDBConfig(replicas, ReplicaInfo());
}

}  // namespace resdb
