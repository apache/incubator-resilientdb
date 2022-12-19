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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "crypto/key_generator.h"
#include "crypto/signature_verifier.h"

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
