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

#include <getopt.h>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <fstream>

#include "interface/kv/contract_client.h"
#include "platform/config/resdb_config_utils.h"

using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;
using resdb::contract::ContractClient;

bool IsValidAddress(const std::string& address) {
    if (address.length() != 42) {
        return false;
    }

    if (address.substr(0, 2) != "0x") {
        return false;
    }

    for (size_t i = 2; i < address.length(); ++i) {
        if (!std::isxdigit(address[i])) {
            return false;
        }
    }
    return true;
}

void ShowUsage() {
  printf(
      "<cmd> -c <config> -m <caller address> -n <contract name> -p <contact "
      "path> -a <params> \n");
  exit(0);
}

static struct option long_options[] = {
    { "cmd", required_argument, NULL, 'm'},
    { "config_file", required_argument, NULL, 'f'},
    { 0, 0, 0, 0 }
};


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

nlohmann::json ReadJSConfig(const std::string& config_path) {

  std::ifstream contract_fstream(config_path);
  if (!contract_fstream) {
    throw std::runtime_error( "Unable to open config file "+config_path);
  }

  return nlohmann::json::parse(contract_fstream);
}

std::string GetValue(const nlohmann::json& js, std::string key){
if(!js.contains(key)){
      printf("need %s\n", key.c_str());
      exit(0);
    }
    return js[key];
}


int main(int argc, char** argv) {
  if (argc < 2) {
    printf("<cmd> -c [config]\n");
    return 0;
  }


  std::string config_file;
  
  std::string cmd;
  std::string caller_address, contract_name, contract_path, params,
      contract_address, func_name;
  int c;
  int option_index;
  std::string client_config_file;
  while ((c = getopt_long(argc, argv, "c:h", long_options, &option_index)) != -1) {
    switch (c) {
      case -1:
        break;
      case 'f':
        config_file = optarg;
        break;
      case 'c':
        client_config_file = optarg;
        break;
      case 'h':
        ShowUsage();
        break;
    }
  }

  nlohmann::json js = ReadJSConfig(config_file);
  std::cout<<js<<std::endl;
  cmd = GetValue(js, "command");

  ResDBConfig config = GenerateResDBConfig(client_config_file);
  config.SetClientTimeoutMs(100000);

  ContractClient client(config);
  printf("cmd = %s\n", cmd.c_str());
  if (cmd == "create_account") {
    CreateAccount(&client);
  } else if (cmd == "deploy") {
    
    contract_path = GetValue(js, "contract_path");
    contract_name = GetValue(js, "contract_name");
    contract_address = GetValue(js, "owner_address");
    params = GetValue(js, "init_params");

    if (!IsValidAddress(contract_address)) {
        printf("ERROR: Invalid owner address format: %s\n", contract_address.c_str());
        return 1;
    }

    printf("contract path %s cmd %s contract name %s caller_address %s init params %s\n", contract_path.c_str(), cmd.c_str(), contract_name.c_str(), contract_address.c_str(), params.c_str());

    std::vector<std::string> init_params;
    boost::split(init_params, params, boost::is_any_of(","));

    DeployContract(&client, contract_address, contract_name, contract_path,
                   init_params);
  } else if (cmd == "execute") {

    caller_address = GetValue(js, "caller_address");
    contract_address = GetValue(js, "contract_address");
    func_name = GetValue(js, "func_name");
    params = GetValue(js, "params");

    if (!IsValidAddress(caller_address)) {
        printf("ERROR: Invalid caller address format: %s\n", caller_address.c_str());
        return 1;
    }


    if (!IsValidAddress(contract_address)) {
        printf("ERROR: Invalid contract address format: %s\n", contract_address.c_str());
        return 1;
    }


    printf(
        "execute\n caller address:%s\n contract address: %s\n func: %s\n "
        "params:%s\n",
        caller_address.c_str(), contract_address.c_str(), func_name.c_str(),
        params.c_str());
    std::vector<std::string> func_params;
    boost::split(func_params, params, boost::is_any_of(","));

    ExecuteContract(&client, caller_address, contract_address, func_name,
                    func_params);
  }
  else if (cmd == "get_balance") {
    std::string address = GetValue(js, "address");

    // Validate address format
    if (!IsValidAddress(address)) {
        printf("ERROR: Invalid address format: %s\n", address.c_str());
        return 1;
    }

    auto balance_or = client.GetBalance(address);
    printf("get address %s balance %s\n", address.c_str(), (*balance_or).c_str());
  } else if (cmd == "set_balance") {
    std::string address = GetValue(js, "address");
    std::string balance = GetValue(js, "balance");

    // Validate address format
    if (!IsValidAddress(address)) {
        printf("ERROR: Invalid address format: %s\n", address.c_str());
        return 1;
    }

    printf("address %s balance %s\n", address.c_str(), balance.c_str());
    auto ret = client.SetBalance(address, balance);
    printf("set address %s balance %s ret %s\n", address.c_str(), balance.c_str(), (*ret).c_str());
  }

}

