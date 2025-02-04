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

#include <filesystem>
#include <fstream>

#include "chain/storage/res_leveldb.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/statistic/stats.h"
#include "service/contract/benchmark/generator/string_generator.h"
#include "service/contract/benchmark/generator/zipfian_generator.h"
#include "service/contract/tusk_service/contract_transaction_manager.h"
#include "service/contract/tusk_service/tusk_service.h"
#include "service/utils/server_factory.h"

using namespace resdb;
using resdb::ConsensusManagerPBFT;
using resdb::CustomGenerateResDBServer;
using resdb::GenerateResDBConfig;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::ServiceNetwork;
using resdb::Stats;
using resdb::Storage;
using resdb::contract::Account;
using resdb::contract::ContractTransactionManager;
using resdb::contract::DeployInfo;
using resdb::contract::FuncInfo;
using resdb::contract::Params;
using resdb::contract::Tusk;
using resdb::contract::x_manager::AddressManager;

void ShowUsage() {
  printf(
      "<config> <private_key> <cert_file> <durability_option> [logging_dir]\n");
}

std::string get_home_dir() { return std::filesystem::path(getenv("HOME")); }

//#define USE_TPCC
#define USE_SMALLBANK

#ifdef USE_TPCC
std::unique_ptr<StringGenerator> wgenerator, dgenerator;
#endif

int pr = 0;

std::vector<std::string> gaddress_list;
Params GetFuncParams(StringGenerator* generator) {
  Params func_params;
#ifdef USE_SMALLBANK
  int p = rand() % 100;
  if (p < pr) {
    func_params.set_func_name("getBalance(address)");
    func_params.add_param(generator->Next());
    // func_params.add_param(std::to_string(rand()%1000));
    func_params.set_is_only(true);
  } else {
    func_params.set_func_name("sendPayment(address,address,uint256)");
    func_params.add_param(generator->Next());
    func_params.add_param(generator->Next());
    func_params.add_param(std::to_string(rand() % 1000));
  }
#endif

#ifdef USE_TPCC
  func_params.set_func_name("payment(address,address,address,uint256)");
  // func_params.add_param(std::to_string(1));
  // func_params.add_param(wgenerator->Next());
  func_params.add_param(gaddress_list[0]);
  func_params.add_param(dgenerator->Next());
  func_params.add_param(generator->Next());
  // func_params.add_param(dgenerator->Next());
  // func_params.add_param(gaddress_list[0]);
  func_params.add_param(std::to_string(rand() % 1000));
#endif
  return func_params;
}

std::string CreateAccount(const std::string& address) {
  resdb::contract::Request request;
  request.set_cmd(resdb::contract::Request::CREATE_ACCOUNT);
  request.set_caller_address(address);
  std::string resp;
  request.SerializeToString(&resp);
  return resp;
}

std::string Deploy(const std::string& contract_path,
                   const std::string& contract_address,
                   const std::string& owner) {
  std::ifstream contract_fstream(contract_path);
  if (!contract_fstream) {
    LOG(ERROR) << "read path fail:" << contract_path;
    throw std::runtime_error(fmt::format(
        "Unable to open contract definition file {}", contract_path));
  }

  nlohmann::json definition = nlohmann::json::parse(contract_fstream);

  auto contracts_json = definition["contracts"];
#ifdef USE_TPCC
  std::string contract_name = "tpcc.sol:TPCC";
#endif
#ifdef USE_SMALLBANK
  std::string contract_name = "smallbank.sol:SmallBank";
#endif
  std::string contract_code = contracts_json[contract_name]["bin"];
  nlohmann::json func_hashes = contracts_json[contract_name]["hashes"];

  // deploy
  DeployInfo deploy_info;
  deploy_info.set_contract_bin(contract_code);
  deploy_info.set_contract_name(contract_name);

  for (auto& func : func_hashes.items()) {
    FuncInfo* new_func = deploy_info.add_func_info();
    new_func->set_func_name(func.key());
    new_func->set_hash(func.value());
  }
  deploy_info.add_init_param("1000");

  resdb::contract::Request request;

  request.set_caller_address(owner);
  request.set_contract_address(contract_address);
  *request.mutable_deploy_info() = deploy_info;
  request.set_cmd(resdb::contract::Request::DEPLOY);

  std::string resp;
  request.SerializeToString(&resp);
  return resp;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    exit(0);
  }

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];
  char* logging_dir = nullptr;

#ifdef USE_SMALLBANK
  std::string contract_path = get_home_dir() + "/smallbank.json";
#endif

#ifdef USE_TPCC
  std::string contract_path = get_home_dir() + "/tpcc.json";
#endif
  // std::string contract_path = argv[4];

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);
  ResConfigData config_data = config->GetConfigData();

  config->RunningPerformance(true);

  // std::unique_ptr<Storage> storage = NewResLevelDB(cert_file, config_data);

  auto performance_consens = std::make_unique<Tusk>(
      *config, std::make_unique<ContractTransactionManager>(nullptr));

  double alpha = 0.85;
#ifdef USE_TPCC
  int user_num = 3000;
#endif

#ifdef USE_SMALLBANK
  int user_num = 100;
#endif
  int id = config->GetSelfInfo().id();
  srand(id * id + id);

  auto generator = std::make_unique<StringGenerator>(
      std::make_unique<ZipfianGenerator>(user_num, alpha));

#ifdef USE_TPCC
  wgenerator = std::make_unique<StringGenerator>(
      std::make_unique<ZipfianGenerator>(user_num, alpha));
  dgenerator = std::make_unique<StringGenerator>(
      std::make_unique<ZipfianGenerator>(user_num, alpha));
#endif

  auto address_manager = std::make_unique<AddressManager>();
  std::string owner =
      AddressManager::AddressToHex(address_manager->CreateRandomAddress());

  std::string contract_address =
      AddressManager::AddressToHex(address_manager->CreateRandomAddress());

  std::vector<std::string> address_list;
  for (int i = 0; i < user_num; ++i) {
    address_list.push_back(
        AddressManager::AddressToHex(address_manager->CreateRandomAddress()));
    gaddress_list.push_back(
        AddressManager::AddressToHex(address_manager->CreateRandomAddress()));
  }

  for (const std::string& addr : address_list) {
    generator->AddString(addr);
  }

#ifdef USE_TPCC
  {
    std::vector<std::string> address_list;
    for (int i = 0; i < 1; ++i) {
      address_list.push_back(
          AddressManager::AddressToHex(address_manager->CreateRandomAddress()));
    }

    for (const std::string& addr : address_list) {
      wgenerator->AddString(addr);
    }
  }
  {
    std::vector<std::string> address_list;
    for (int i = 0; i < 10; ++i) {
      address_list.push_back(
          AddressManager::AddressToHex(address_manager->CreateRandomAddress()));
    }

    for (const std::string& addr : address_list) {
      dgenerator->AddString(addr);
    }
  }

#endif

  performance_consens->SetupPerformancePreprocessFunc([&]() {
    LOG(ERROR) << "preprocess";
    std::vector<std::string> txns;

    txns.push_back(CreateAccount(owner));
    LOG(ERROR) << "create account";
    txns.push_back(Deploy(contract_path, contract_address, owner));
    LOG(ERROR) << "contract";
    return txns;
  });

  performance_consens->SetupPerformanceDataFunc([&]() {
    resdb::contract::Request request;
    Params func_params = GetFuncParams(generator.get());
    // LOG(ERROR)<<"start function:"<<func_params.func_name();

    request.set_caller_address(owner);
    request.set_contract_address(contract_address);
    request.set_cmd(resdb::contract::Request::EXECUTE);
    *request.mutable_func_params() = func_params;

    std::string request_data;
    request.SerializeToString(&request_data);
    // LOG(ERROR)<<"start function done:"<<func_params.func_name();
    return request_data;
  });

  auto server =
      std::make_unique<ServiceNetwork>(*config, std::move(performance_consens));

  server->Run();
}
