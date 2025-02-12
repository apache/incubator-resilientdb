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

#include <glog/logging.h>

#include <boost/algorithm/string.hpp>
#include <vector>
#include <unistd.h>  // For getopt

#include "interface/contract/contract_client.h"
#include "platform/config/resdb_config_utils.h"

using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;
using resdb::contract::ContractClient;

void ShowUsage() {
  printf(
      "<cmd> -c <config> -m <caller address> -n <contract name> -p <contract "
      "path> -a <params> -e <external address>\n");
  exit(0);
}

void AddAddress(ContractClient* client, const std::string& external_address) {
  absl::Status status = client->AddExternalAddress(external_address);
  if (!status.ok()) {
    printf("Add address failed\n");
  } else {
    printf("Address added successfully\n");
  }
}

void CreateAccount(ContractClient* client) {
  auto account = client->CreateAccount();
  if (!account.ok()) {
    printf("create account fail\n");
    return;
  }
  LOG(ERROR) << "create account:\n" << account->DebugString();
}

void DeployContract(ContractClient* client, const std::string& caller_address,
                    const std::string& contract_name,
                    const std::string& contract_path,
                    const std::vector<std::string>& init_params) {
  auto contract = client->DeployContract(caller_address, contract_name,
                                         contract_path, init_params);
  if (!contract.ok()) {
    printf("deploy contract fail\n");
    return;
  }
  LOG(ERROR) << "deploy contract:\n" << contract->DebugString();
}

void ExecuteContract(ContractClient* client, const std::string& caller_address,
                     const std::string& contract_address,
                     const std::string& func_name,
                     const std::vector<std::string>& params) {
  auto output = client->ExecuteContract(caller_address, contract_address,
                                        func_name, params);
  if (!output.ok()) {
    printf("execute contract fail\n");
    return;
  }
  LOG(ERROR) << "execute result:\n" << *output;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    ShowUsage();
    return 0;
  }

  std::string cmd = argv[1];
  std::string caller_address, contract_name, contract_path, params,
      contract_address, func_name, external_address;  // Added external_address
  int c;
  std::string client_config_file;
  while ((c = getopt(argc, argv, "m:c:a:n:p:h:f:s:e:")) != -1) {  // Added 'e:'
    switch (c) {
      case 'm':
        caller_address = optarg;
        break;
      case 'c':
        client_config_file = optarg;
        break;
      case 'n':
        contract_name = optarg;
        break;
      case 'f':
        func_name = optarg;
        break;
      case 'p':
        contract_path = optarg;
        break;
      case 'a':
        params = optarg;
        break;
      case 's':
        contract_address = optarg;
        break;
      case 'e':
        external_address = optarg;  // Handle the 'e' option
        break;
      case 'h':
        ShowUsage();
        break;
      default:
        ShowUsage();
        break;
    }
  }

  printf("cmd = %s config path = %s\n", cmd.c_str(),
         client_config_file.c_str());
  ResDBConfig config = GenerateResDBConfig(client_config_file);
  config.SetClientTimeoutMs(100000);

  ContractClient client(config);

  if (cmd == "create") {
    CreateAccount(&client);
  } else if (cmd == "add_address") {
    AddAddress(&client, external_address);
  } else if (cmd == "deploy") {
    std::vector<std::string> init_params;
    boost::split(init_params, params, boost::is_any_of(","));

    DeployContract(&client, caller_address, contract_name, contract_path,
                   init_params);
  } else if (cmd == "execute") {
    printf(
        "execute\n caller address:%s\n contract address: %s\n func: %s\n "
        "params:%s\n",
        caller_address.c_str(), contract_address.c_str(), func_name.c_str(),
        params.c_str());
    std::vector<std::string> func_params;
    boost::split(func_params, params, boost::is_any_of(","));

    ExecuteContract(&client, caller_address, contract_address, func_name,
                    func_params);
  } else {
    ShowUsage();
  }
}