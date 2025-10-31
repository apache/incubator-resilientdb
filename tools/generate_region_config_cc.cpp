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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <google/protobuf/util/json_util.h>
#include "platform/proto/replica_info.pb.h"

namespace {

using google::protobuf::util::JsonStringToMessage;
using google::protobuf::util::MessageToJsonString;
using google::protobuf::util::JsonParseOptions;
using resdb::ResConfigData;
using resdb::ReplicaInfo;
using resdb::RegionInfo;

bool readLines(const std::string& path, std::vector<std::string>* lines) {
  std::ifstream in(path);
  if (!in.is_open()) {
    std::cerr << "Failed to open file: " << path << "\n";
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    lines->push_back(line);
  }
  return true;
}

std::string readFileToString(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    std::cerr << "Failed to open file: " << path << "\n";
    return "";
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <server.config input> <output.json> [template.json]\n";
    return 1;
  }

  const std::string input_path = argv[1];
  const std::string output_path = argv[2];
  const std::string template_path = (argc > 3 ? argv[3] : "");

  std::vector<std::string> lines;
  if (!readLines(input_path, &lines)) {
    std::cerr << "Failed to read input file: " << input_path << "\n";
    return 2;
  }

  ResConfigData config_data;
  std::map<std::string, std::vector<ReplicaInfo>> region_to_replicas;
  
  for (const std::string& line : lines) {
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }
    
    std::istringstream iss(line);
    std::string id_str, ip, port_str, region_id;
    
    if (!(iss >> id_str >> ip >> port_str)) {
      continue;
    }
    
    if (!(iss >> region_id)) {
      region_id = "0";
    }
    
    int64_t id;
    int32_t port;
    try {
      id = std::stoll(id_str);
      port = std::stoi(port_str);
    } catch (const std::exception& e) {
      std::cerr << "Warning: Failed to parse id or port from line: " << line << "\n";
      continue;
    }
    
    ReplicaInfo replica;
    replica.set_id(id);
    replica.set_ip(ip);
    replica.set_port(port);
    
    region_to_replicas[region_id].push_back(replica);
  }

  for (auto& [rid, replicas] : region_to_replicas) {
    RegionInfo* region = config_data.add_region();
    for (auto& replica : replicas) {
      *region->add_replica_info() = replica;
    }
  }

  std::string output_json;
  MessageToJsonString(config_data, &output_json);
  std::cerr << "Generated JSON (before template):\n" << output_json << "\n\n";

  std::ofstream out(output_path);
  if (!out.is_open()) {
    std::cerr << "Failed to open output file: " << output_path << "\n";
    return 4;
  }
  out << output_json;
  out.close();

  if (!template_path.empty()) {
    std::string base_json = readFileToString(output_path);
    std::cerr << "Read base config:\n" << base_json << "\n\n";
    
    ResConfigData base_config;
    JsonParseOptions parse_options;
    auto status = JsonStringToMessage(base_json, &base_config, parse_options);
    if (!status.ok()) {
      std::cerr << "Failed to parse generated JSON: " << status.message() << "\n";
      return 5;
    }
    
    std::string template_json_str = readFileToString(template_path);
    if (template_json_str.empty()) {
      std::cerr << "Failed to read template file: " << template_path << "\n";
      return 6;
    }
    std::cerr << "Read template config:\n" << template_json_str << "\n\n";
    
    ResConfigData template_config;
    status = JsonStringToMessage(template_json_str, &template_config, parse_options);
    if (!status.ok()) {
      std::cerr << "Failed to parse template JSON: " << status.message() << "\n";
      return 7;
    }
    
    base_config.MergeFrom(template_config);
    
    std::string merged_json;
    MessageToJsonString(base_config, &merged_json);
    std::cerr << "Merged JSON (after template):\n" << merged_json << "\n\n";
    std::ofstream out2(output_path);
    if (!out2.is_open()) {
      std::cerr << "Failed to reopen output file for merged config\n";
      return 8;
    }
    out2 << merged_json;
    out2.close();
  }

  return 0;
}
