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

#include <glog/logging.h>

#include <boost/algorithm/string.hpp>
#include <vector>

#include "application/contract/client/contract_client.h"
#include "config/resdb_config_utils.h"

using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;
using resdb::contract::ContractClient;

void ShowUsage() {
  printf(
      "<cmd> -c <config> -m <caller address> -n <contract name> -p <contact "
      "path> -a <params> \n");
  exit(0);
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
    printf("<cmd> -c [config]\n");
    return 0;
  }

  std::string cmd = argv[1];
  std::string caller_address, contract_name, contract_path, params,
      contract_address, func_name;
  int c;
  std::string client_config_file;
  while ((c = getopt(argc, argv, "m:c:a:n:p:h:f:s:")) != -1) {
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
      case 'h':
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
  }
}
