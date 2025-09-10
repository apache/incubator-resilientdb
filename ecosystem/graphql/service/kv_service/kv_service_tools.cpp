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

#include <fstream>

#include "common/proto/signature_info.pb.h"
#include "platform/config/resdb_config_utils.h"
#include "service/kv_service/resdb_kv_client.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using sdk::ResDBKVClient;

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("<config path> <cmd>(set/get/getallvalues/getrange), [key] "
           "[value/key2]\n");
    return 0;
  }
  std::string client_config_file = argv[1];
  std::string cmd = argv[2];
  std::string key;
  if (cmd != "getallvalues") {
    key = argv[3];
  }
  std::string value;
  if (cmd == "set") {
    value = argv[4];
  }

  std::string key2;
  if (cmd == "getrange") {
    key2 = argv[4];
  }

  ResDBConfig config = GenerateResDBConfig(client_config_file);

  config.SetClientTimeoutMs(100000);

  ResDBKVClient client(config);

  if (cmd == "set") {
    int ret = client.Set(key, value);
    printf("client set ret = %d\n", ret);
  } else if (cmd == "get") {
    auto res = client.Get(key);
    if (res != nullptr) {
      printf("client get value = %s\n", res->c_str());
    } else {
      printf("client get value fail\n");
    }
  } else if (cmd == "getallvalues") {
    auto res = client.GetAllValues();
    if (res != nullptr) {
      printf("client getallvalues value = %s\n", res->c_str());
    } else {
      printf("client getallvalues value fail\n");
    }
  } else if (cmd == "getrange") {
    auto res = client.GetRange(key, key2);
    if (res != nullptr) {
      printf("client getrange value = %s\n", res->c_str());
    } else {
      printf("client getrange value fail\n");
    }
  }
}
