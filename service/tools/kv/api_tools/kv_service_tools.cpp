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

int main(int argc, char** argv) {
  if (argc < 3) {
    ShowUsage();
    return 0;
  }
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
        printf("current value = %s\n", key.c_str(), res->DebugString().c_str());
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
