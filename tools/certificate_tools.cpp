/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/crypto/key_generator.h"
#include "common/crypto/signature_verifier.h"

using namespace resdb;

void WriteKey(const std::string& key, const std::string& file_name) {
  printf("save key to path %s\n", file_name.c_str());
  int fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) {
    printf("open file %s fail %s\n", file_name.c_str(), strerror(errno));
    return;
  }
  assert(fd >= 0);
  write(fd, key.c_str(), key.size());
  close(fd);
}

KeyInfo ReadKey(const std::string& file_name) {
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd < 0) {
    printf("open file %s fail %s\n", file_name.c_str(), strerror(errno));
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

CertificateInfo GenerateCertificateInfo(const KeyInfo& admin_pub_key,
                                        const KeyInfo& pri_key,
                                        const KeyInfo& pub_key,
                                        int64_t node_id) {
  CertificateInfo info;
  info.set_node_id(node_id);
  *info.mutable_admin_public_key() = admin_pub_key;
  return info;
}

int main(int argc, char** argv) {
  std::string path = "./";
  std::string admin_pri_key_path, admin_pub_key_path;
  std::string node_pub_key_path;
  std::string ip;
  std::string type;
  int port = 0;
  int64_t node_id = 0;
  if (argc < 9) {
    printf(
        "<save path> <administor private key path> <adminisotr public key "
        "path> <node public key path> <node id> <ip> <port> "
        "<type>(client/server)\n");
    exit(0);
  } else {
    path = argv[1];
    admin_pri_key_path = argv[2];
    admin_pub_key_path = argv[3];
    node_pub_key_path = argv[4];
    node_id = strtoull(argv[5], NULL, 10);
    ip = argv[6];
    port = strtoull(argv[7], NULL, 10);
    type = argv[8];
  }
  // Read the keys of current node.
  KeyInfo pub_key = ReadKey(node_pub_key_path);

  // Read the keys of administor.
  KeyInfo admin_pri_key = ReadKey(admin_pri_key_path);
  KeyInfo admin_pub_key = ReadKey(admin_pub_key_path);

  CertificateInfo info;
  info.mutable_admin_public_key()->set_key(admin_pub_key.key());
  info.mutable_admin_public_key()->set_hash_type(admin_pub_key.hash_type());
  *info.mutable_public_key()->mutable_public_key_info()->mutable_key() =
      pub_key;
  info.mutable_public_key()->mutable_public_key_info()->set_node_id(node_id);
  if ("client" == type) {
    info.mutable_public_key()->mutable_public_key_info()->set_type(
        CertificateKeyInfo::CLIENT);
  } else {
    info.mutable_public_key()->mutable_public_key_info()->set_type(
        CertificateKeyInfo::REPLICA);
  }
  info.mutable_public_key()->mutable_public_key_info()->set_ip(ip);
  info.mutable_public_key()->mutable_public_key_info()->set_port(port);

  info.set_node_id(node_id);

  // Get certificate using the key from administor.
  SignatureVerifier verifier(
      admin_pri_key,
      GenerateCertificateInfo(admin_pub_key, admin_pri_key, admin_pub_key, 0));
  auto resp_info =
      verifier.SignCertificateKeyInfo(info.public_key().public_key_info());
  assert(resp_info.ok());
  *info.mutable_public_key()->mutable_certificate() = *resp_info;
  std::cout << "info:" << info.DebugString();
  // Write the keys with certificate to the file.
  std::string str;
  assert(info.SerializeToString(&str));
  WriteKey(str, path + "/cert_" + std::to_string(node_id) + ".cert");
}
