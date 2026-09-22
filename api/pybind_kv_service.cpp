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
#include <pybind11/stl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

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

namespace {

KVClient MakeClient(const std::string& config_path) {
  ResDBConfig config = GenerateResDBConfig(config_path);
  config.SetClientTimeoutMs(100000);
  return KVClient(config);
}

}  // namespace

// Index commands. Return 0 on success, -3 if the servers rejected the
// request, or a send error (-1, -2). Attributes may be a list or, for a
// single-attribute index, a plain string.
int create_index_entry(std::string index_name,
                       std::vector<std::string> attributes,
                       std::string primary_key, std::string config_path) {
  return MakeClient(config_path)
      .CreateIndexEntry(index_name, attributes, primary_key);
}

int create_index_entry_one(std::string index_name, std::string attribute,
                           std::string primary_key, std::string config_path) {
  return create_index_entry(index_name, {attribute}, primary_key, config_path);
}

int delete_index_entry(std::string index_name,
                       std::vector<std::string> attributes,
                       std::string primary_key, std::string config_path) {
  return MakeClient(config_path)
      .DeleteIndexEntry(index_name, attributes, primary_key);
}

int delete_index_entry_one(std::string index_name, std::string attribute,
                           std::string primary_key, std::string config_path) {
  return delete_index_entry(index_name, {attribute}, primary_key, config_path);
}

int update_index_entry(std::string index_name,
                       std::vector<std::string> old_attributes,
                       std::vector<std::string> new_attributes,
                       std::string primary_key, std::string config_path) {
  return MakeClient(config_path)
      .UpdateIndexEntry(index_name, old_attributes, new_attributes,
                        primary_key);
}

int update_index_entry_one(std::string index_name, std::string old_attribute,
                           std::string new_attribute, std::string primary_key,
                           std::string config_path) {
  return update_index_entry(index_name, {old_attribute}, {new_attribute},
                            primary_key, config_path);
}

// Primary keys whose entry starts with `attributes`, each once and sorted.
// An empty list means the whole index.
std::vector<std::string> query_by_index(std::string index_name,
                                        std::vector<std::string> attributes,
                                        std::string config_path) {
  std::vector<std::string> keys;
  auto items = MakeClient(config_path).QueryByIndex(index_name, attributes);
  if (items == nullptr) {
    return keys;
  }
  for (const auto& item : items->item()) {
    keys.push_back(item.key());
  }
  return keys;
}

std::vector<std::string> query_by_index_one(std::string index_name,
                                            std::string attribute,
                                            std::string config_path) {
  return query_by_index(index_name, {attribute}, config_path);
}

PYBIND11_MODULE(pybind_kv, m) {
  m.def("get", &get, "A function that gets a value from the key-value store");
  m.def("set", &set, "A function that sets a value in the key-value store");
  // Each command has a single-string and a list overload: pybind never turns a
  // str into a list of strings, so "Davis" needs its own signature.
  m.def("create_index_entry", &create_index_entry_one,
        "Add an index entry with one attribute");
  m.def("create_index_entry", &create_index_entry,
        "Add an index entry with a list of attributes");
  m.def("delete_index_entry", &delete_index_entry_one,
        "Remove an index entry with one attribute");
  m.def("delete_index_entry", &delete_index_entry,
        "Remove an index entry with a list of attributes");
  m.def("update_index_entry", &update_index_entry_one,
        "Move an index entry to a new attribute");
  m.def("update_index_entry", &update_index_entry,
        "Move an index entry to new attributes");
  m.def("query_by_index", &query_by_index_one,
        "Primary keys matching one attribute");
  m.def("query_by_index", &query_by_index,
        "Primary keys matching a list of attributes");
}
