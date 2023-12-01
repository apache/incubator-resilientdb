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
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>

#include "common/proto/signature_info.pb.h"
#include "interface/kv/kv_client.h"
#include "platform/config/resdb_config_utils.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::KVClient;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;

void ShowUsage() {
  printf(
      "--config: config path\n"
      "--cmd "
      "set/get/set_with_version/get_with_version/get_key_range/"
      "get_key_range_with_version/get_top/get_history\n"
      "--key key\n"
      "--value value, if cmd is a get operation\n"
      "--version version of the value, if cmd is vesion based\n"
      "--min_key the min key if cmd is get_key_range\n"
      "--max_key the max key if cmd is get_key_range\n"
      "--min_version, if cmd is get_history\n"
      "--max_version, if cmd is get_history\n"
      "--top, if cmd is get_top\n"
      "\n"
      "More examples can be found from README.\n");
}

static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"config", required_argument, NULL, 'c'},
    {"cmd", required_argument, NULL, 'f'},
    {"key", required_argument, NULL, 'K'},
    {"value", required_argument, NULL, 'V'},
    {"version", required_argument, NULL, 'v'},
    {"min_version", required_argument, NULL, 's'},
    {"max_version", required_argument, NULL, 'S'},
    {"min_key", required_argument, NULL, 'y'},
    {"max_key", required_argument, NULL, 'Y'},
    {"top", required_argument, NULL, 't'},
};

void OldAPI(char** argv) {
  std::string client_config_file = argv[1];
  std::string cmd = argv[2];
  std::string key;
  if (cmd != "getvalues") {
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

  KVClient client(config);

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
  } else if (cmd == "getvalues") {
    auto res = client.GetAllValues();
    if (res != nullptr) {
      printf("client getvalues value = %s\n", res->c_str());
    } else {
      printf("client getvalues value fail\n");
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

int main(int argc, char** argv) {
  std::string key;
  int version = -1;
  int option_index = 0;
  int min_version = -1, max_version = -1;
  std::string min_key, max_key;
  std::string value;
  std::string client_config_file;
  int top = 0;
  char c;
  std::string cmd;

  if (argc >= 3) {
    cmd = argv[2];
    if (cmd == "get" || cmd == "set" || cmd == "getvalues" ||
        cmd == "getrange") {
      OldAPI(argv);
      return 0;
    }
  }

  while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) !=
         -1) {
    switch (c) {
      case 'c':
        client_config_file = optarg;
        break;
      case 'f':
        cmd = optarg;
        break;
      case 'K':
        key = optarg;
        break;
      case 'V':
        value = optarg;
        break;
      case 'v':
        version = strtoull(optarg, NULL, 10);
        break;
      case 's':
        min_version = strtoull(optarg, NULL, 10);
        break;
      case 'S':
        max_version = strtoull(optarg, NULL, 10);
        break;
      case 'y':
        min_key = optarg;
        break;
      case 'Y':
        max_key = optarg;
        break;
      case 't':
        top = strtoull(optarg, NULL, 10);
        break;
      case 'h':
        ShowUsage();
        break;
    }
  }

  ResDBConfig config = GenerateResDBConfig(client_config_file);

  config.SetClientTimeoutMs(100000);
  KVClient client(config);
  if (cmd == "set_with_version") {
    if (key.empty() || value.empty() || version < 0) {
      ShowUsage();
      return 0;
    }
    int ret = client.Set(key, value, version);
    printf("set key = %s, value = %s, version = %d done, ret = %d\n",
           key.c_str(), value.c_str(), version, ret);
    if (ret == 0) {
      usleep(100000);
      auto res = client.Get(key, 0);
      if (res != nullptr) {
        printf("current value = %s\n", res->DebugString().c_str());
      } else {
        printf("get value fail\n");
      }
    }
  } else if (cmd == "set") {
    if (key.empty() || value.empty()) {
      ShowUsage();
      return 0;
    }
    int ret = client.Set(key, value);
    printf("set key = %s, value = %s done, ret = %d\n", key.c_str(),
           value.c_str(), ret);
  } else if (cmd == "get_with_version") {
    auto res = client.Get(key, version);
    if (res != nullptr) {
      printf("get key = %s, value = %s\n", key.c_str(),
             res->DebugString().c_str());
    } else {
      printf("get value fail\n");
    }
  } else if (cmd == "get") {
    auto res = client.Get(key);
    if (res != nullptr) {
      printf("get key = %s value = %s\n", key.c_str(), res->c_str());
    } else {
      printf("get value fail\n");
    }
  } else if (cmd == "get_top") {
    auto res = client.GetKeyTopHistory(key, top);
    if (res != nullptr) {
      printf("key = %s, top %d\n value = %s\n", key.c_str(), top,
             res->DebugString().c_str());
    } else {
      printf("get key = %s top %d value fail\n", key.c_str(), top);
    }
  } else if (cmd == "get_history") {
    if (key.empty() || min_version < 0 || max_version < 0 ||
        max_version < min_version) {
      ShowUsage();
      return 0;
    }
    auto res = client.GetKeyHistory(key, min_version, max_version);
    if (res != nullptr) {
      printf(
          "get history key = %s, min version = %d, max version = %d\n value = "
          "%s\n",
          key.c_str(), min_version, max_version, res->DebugString().c_str());
    } else {
      printf(
          "get history key = %s, min version = %d, max version = %d value "
          "fail\n",
          key.c_str(), min_version, max_version);
    }
  } else if (cmd == "get_key_range") {
    if (min_key.empty() || max_key.empty() || min_key > max_key) {
      ShowUsage();
      return 0;
    }
    auto res = client.GetRange(min_key, max_key);
    if (res != nullptr) {
      printf("getrange min key = %s, max key = %s\n value = %s\n",
             min_key.c_str(), max_key.c_str(), (*res).c_str());
    } else {
      printf("getrange value fail, min key = %s, max key = %s\n",
             min_key.c_str(), max_key.c_str());
    }
  } else if (cmd == "get_key_range_with_version") {
    if (min_key.empty() || max_key.empty() || min_key > max_key) {
      ShowUsage();
      return 0;
    }
    printf("min key = %s max key = %s\n", min_key.c_str(), max_key.c_str());
    auto res = client.GetKeyRange(min_key, max_key);
    if (res != nullptr) {
      printf("getrange min key = %s, max key = %s\n value = %s\n",
             min_key.c_str(), max_key.c_str(), res->DebugString().c_str());
    } else {
      printf("getrange value fail, min key = %s, max key = %s\n",
             min_key.c_str(), max_key.c_str());
    }
  } else {
    ShowUsage();
  }
}
