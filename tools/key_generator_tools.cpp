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

using namespace resdb;

void WriteKey(const KeyInfo& key, const std::string& file_name) {
  std::string str;
  assert(key.SerializeToString(&str));

  printf("save key to path %s\n", file_name.c_str());
  int fd = open(file_name.c_str(), O_WRONLY | O_CREAT, 0666);
  if (fd < 0) {
    printf("open file fail %s\n", strerror(errno));
  }
  assert(fd >= 0);
  write(fd, str.c_str(), str.size());
  close(fd);
}

int main(int argc, char** argv) {
  std::string type = "ED25519";
  std::string path = "./";
  if (argc < 2) {
    printf("<path> [type](RSA|AES|ED25519(default))\n");
    exit(0);
  } else {
    path = argv[1];
    if (argc > 2) {
      type = argv[2];
    }
  }
  SignatureInfo::HashType hash_type;
  if (type == "RSA") {
    hash_type = SignatureInfo::RSA;
  } else if (type == "AES") {
    hash_type = SignatureInfo::CMAC_AES;
  } else if (type == "ED25519") {
    hash_type = SignatureInfo::ED25519;
  } else {
    printf("type invalid\n");
    exit(0);
  }

  SecretKey key = KeyGenerator::GeneratorKeys(hash_type);

  std::string str;
  assert(key.SerializeToString(&str));

  // Save private key and public key to files.
  KeyInfo pub_info, pri_info;
  pub_info.set_key(key.public_key());
  pub_info.set_hash_type(key.hash_type());

  pri_info.set_key(key.private_key());
  pri_info.set_hash_type(key.hash_type());

  WriteKey(pri_info, path + ".key.pri");
  WriteKey(pub_info, path + ".key.pub");
}
