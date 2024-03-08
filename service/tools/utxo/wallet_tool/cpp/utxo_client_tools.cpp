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
#include <google/protobuf/util/json_util.h>

#include "common/crypto/signature_utils.h"
#include "interface/utxo/utxo_client.h"
#include "platform/config/resdb_config_utils.h"

using google::protobuf::util::MessageToJsonString;
using resdb::GenerateResDBConfig;
using resdb::ResDBConfig;
using resdb::utxo::UTXO;
using resdb::utxo::UTXOClient;
using resdb::utxo::UTXOIn;
using resdb::utxo::UTXOOut;

void ShowUsage() {
  printf(
      "<cmd> -c <config> -m <caller address> -n <contract name> -p <contact "
      "path> -a <params> \n");
  exit(0);
}

void Transfer(UTXOClient* client, int64_t transaction_id,
              const std::string& address,
              const std::vector<std::string>& to_address,
              const std::vector<int64_t>& values,
              const std::string& private_key,
              const std::vector<std::string>& to_pub_key) {
  if (private_key.empty() || to_pub_key.empty()) {
    printf("no private key or public key\n");
    return;
  }
  UTXO utxo;
  int64_t nonce = 0;
  UTXOIn* in = utxo.add_in();
  in->set_prev_id(transaction_id);
  in->set_out_idx(0);
  nonce += transaction_id;

  for (size_t i = 0; i < to_address.size(); ++i) {
    UTXOOut* out = utxo.add_out();
    out->set_address(to_address[i]);
    out->set_value(values[i]);
    out->set_pub_key(to_pub_key[i]);
    utxo.set_address(address);
    utxo.set_sig(resdb::utils::ECDSASignString(
        private_key, address + std::to_string(nonce)));
    LOG(ERROR) << "transfer from:" << address << " to:" << to_address[i]
               << " value:" << values[i];
  }

  auto output = client->Transfer(utxo);
  LOG(ERROR) << "execute result:\n" << output;
}

void GetList(UTXOClient* client, int64_t end_id, int num) {
  std::vector<UTXO> list = client->GetList(end_id, num);
  LOG(ERROR) << "get utxo:";
  for (const UTXO& utxo : list) {
    std::string str;
    MessageToJsonString(utxo, &str);
    printf("%s\n", str.c_str());
  }
}

void GetWallet(UTXOClient* client, const std::string& address) {
  int64_t ret = client->GetWallet(address);
  LOG(ERROR) << "address:" << address << " get wallet value:" << ret;
}

std::vector<std::string> ParseString(std::string str) {
  std::vector<std::string> ret;
  while (true) {
    size_t pos = str.find(",");
    if (pos == std::string::npos) {
      ret.push_back(str);
      break;
    }
    ret.push_back(str.substr(0, pos));
    str = str.substr(pos + 1);
  }
  return ret;
}

std::vector<int64_t> ParseValue(std::string str) {
  std::vector<int64_t> ret;
  while (true) {
    size_t pos = str.find(",");
    if (pos == std::string::npos) {
      ret.push_back(strtoull(str.c_str(), NULL, 10));
      break;
    }
    ret.push_back(strtoull(str.substr(0, pos).c_str(), NULL, 10));
    str = str.substr(pos + 1);
  }
  return ret;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("-d <cmd> -c [config]\n");
    return 0;
  }

  int64_t transaction_id = 0;
  std::string address, to_address, params, contract_address, func_name,
      private_key, to_pub_key;
  int64_t end_id = 0;
  int num = 10;
  int c;
  std::string cmd;
  std::string value;
  std::string client_config_file;
  while ((c = getopt(argc, argv, "c:d:t:x:m:h:e:v:n:p:b:")) != -1) {
    switch (c) {
      case 'c':
        client_config_file = optarg;
        break;
      case 'd':
        address = optarg;
        break;
      case 't':
        to_address = optarg;
        break;
      case 'x':
        transaction_id = strtoull(optarg, NULL, 10);
        break;
      case 'm':
        cmd = optarg;
        break;
      case 'e':
        end_id = strtoull(optarg, NULL, 10);
        break;
      case 'n':
        num = strtoull(optarg, NULL, 10);
        break;
      case 'v':
        value = optarg;
        break;
      case 'p':
        private_key = optarg;
        break;
      case 'b':
        to_pub_key = optarg;
        break;
      case 'h':
        ShowUsage();
        break;
    }
  }

  ResDBConfig config = GenerateResDBConfig(client_config_file);
  config.SetClientTimeoutMs(100000);
  UTXOClient client(config);
  if (cmd == "transfer") {
    Transfer(&client, transaction_id, address, ParseString(to_address),
             ParseValue(value), private_key, ParseString(to_pub_key));
  } else if (cmd == "list") {
    GetList(&client, end_id, num);
  } else if (cmd == "wallet") {
    GetWallet(&client, to_address);
  }
}
