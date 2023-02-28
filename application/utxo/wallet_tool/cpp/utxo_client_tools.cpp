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
#include <google/protobuf/util/json_util.h>

#include "application/utxo/client/utxo_client.h"
#include "config/resdb_config_utils.h"
#include "crypto/signature_utils.h"

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
              const std::string& address, const std::string& to_address,
              const int value, const std::string& private_key,
              const std::string& to_pub_key) {
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

  UTXOOut* out = utxo.add_out();
  out->set_address(to_address);
  out->set_value(value);
  out->set_pub_key(to_pub_key);
  utxo.set_address(address);
  utxo.set_sig(resdb::utils::ECDSASignString(private_key,
                                             address + std::to_string(nonce)));

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
  int64_t value = 0;
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
        value = strtoull(optarg, NULL, 10);
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
    Transfer(&client, transaction_id, address, to_address, value, private_key,
             to_pub_key);
  } else if (cmd == "list") {
    GetList(&client, end_id, num);
  } else if (cmd == "wallet") {
    GetWallet(&client, to_address);
  }
}
