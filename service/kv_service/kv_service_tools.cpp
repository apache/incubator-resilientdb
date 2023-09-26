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

#include <fstream>

#include "common/proto/signature_info.pb.h"
#include "platform/config/resdb_config_utils.h"
#include "service/kv_service/resdb_kv_client.h"

using resdb::GenerateReplicaInfo;
using resdb::GenerateResDBConfig;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;
using sdk::ResDBKVClient;

int main(int argc, char** argv) {
  if (argc < 3) {
    printf(
        "<config path> <cmd>(set/get/getvalues/getrange), [key] "
        "[value/key2]\n");
    return 0;
  }
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
  } else if (cmd == "getvalues") {
    auto res = client.GetValues();
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
