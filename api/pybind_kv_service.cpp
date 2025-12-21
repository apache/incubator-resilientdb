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
#include <pybind11/pybind11.h>
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

std::string get(std::string key, std::string config_path) {
  ResDBConfig config = GenerateResDBConfig(config_path);
  config.SetClientTimeoutMs(100000);
  KVClient client(config);
  auto result_ptr = client.Get(key);
  if (result_ptr) {
    return *result_ptr;
  } else {
    return "";
  }
}

bool set(std::string key, std::string value, std::string config_path) {
  ResDBConfig config = GenerateResDBConfig(config_path);
  config.SetClientTimeoutMs(100000);
  KVClient client(config);
  int result = client.Set(key, value);
  if (result == 0) {
    return true;
  } else {
    return false;
  }
}

PYBIND11_MODULE(pybind_kv, m) {
  m.def("get", &get, "A function that gets a value from the key-value store");
  m.def("set", &set, "A function that sets a value in the key-value store");
}
