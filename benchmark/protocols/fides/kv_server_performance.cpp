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

#include "chain/storage/memory_db.h"
#include "enclave/sgx_cpp_u.h"
#include "executor/kv/kv_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/fides/framework/consensus.h"
#include "platform/networkstrate/service_network.h"
#include "platform/statistic/stats.h"
#include "proto/kv/kv.pb.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sys/timeb.h>

using namespace resdb;
using namespace resdb::fides;
using namespace resdb::storage;


unsigned char* key_buf;
size_t key_len = 0;
oe_enclave_t* enclave = NULL;
oe_result_t result;
int ret = 0;
uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

BIO *bio = BIO_new_mem_buf(key_buf, -1);
RSA *pub_key = NULL;
size_t encrypted_len;
unsigned char* encrypted_data;

bool check_simulate_opt(int* argc, char* argv[]) {
  for (int i = 0; i < *argc; i++) {
    if (strcmp(argv[i], "--simulate") == 0) {
      std::cout << "Running in simulation mode" << std::endl;
      memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
      (*argc)--;
      return true;
    }
  }
  return false;
}

void ShowUsage() {
  printf("<config> <private_key> <cert_file> [logging_dir]\n");
}

std::string GetRandomKey() {
  int num1 = rand() % 10;
  int num2 = rand() % 10;
  return std::to_string(num1) + std::to_string(num2);
}

std::string GetEncryptedRandomKey() {
  int num1 = rand() % 10;
  int num2 = rand() % 10;
  std::string key = std::to_string(num1) + std::to_string(num2);
  std::cout<<"key: "<<key<<std::endl;
  const unsigned char* key_data = reinterpret_cast<const unsigned char*>(key.c_str());

  encrypted_data = new unsigned char[encrypted_len]; // Initialize buffer
  std::cout<<"Before RSA_public_encrypt." << std::endl;
  size_t result_len = RSA_public_encrypt(key.size(), key_data, encrypted_data, 
                                         pub_key, RSA_PKCS1_PADDING);

  std::string encrypted_key(encrypted_data, encrypted_data + result_len);
  std::cout<<"encrypted_key: "<<encrypted_key<<std::endl;
  return encrypted_key;
}

int main(int argc, char** argv) {
  printf("argc: %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }

  if (argc < 3) {
    ShowUsage();
    exit(0);
  }

  // google::InitGoogleLogging(argv[0]);
  // FLAGS_minloglevel = google::GLOG_WARNING;

  char* config_file = argv[1];
  char* private_key_file = argv[2];
  char* cert_file = argv[3];

  char* enclave_file = argv[4];

  if (check_simulate_opt(&argc, argv)) {
    flags |= OE_ENCLAVE_FLAG_SIMULATE;
  }
  result = oe_create_sgx_cpp_enclave(enclave_file, OE_ENCLAVE_TYPE_SGX, flags,
                                     NULL, 0, &enclave);
  if (result != OE_OK) {
    std::cerr << "oe_create_sgx_cpp_enclave() failed with " << argv[0] << " "
              << result << std::endl;
    ret = 1;
    // goto exit;
  }

  if (argc >= 6) {
    auto monitor_port = Stats::GetGlobalStats(5);
    monitor_port->SetPrometheus(argv[5]);
  }

  printf("Run 1\n");
  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file);

  config->RunningPerformance(true);
  ResConfigData config_data = config->GetConfigData();

  std::cout << "kv_server: reseting prng" << std::endl;
  // Predefined key, can replace it by using some DKG protocol
  uint32_t rdrandNum = 123456789; 
  uint32_t totalNum = config->GetReplicaNum();
  result = reset_prng(enclave, &ret, &rdrandNum, &totalNum);
  if (result != OE_OK) {
    ret = 1;
  }
  if (ret != 0) {
    std::cerr << "kv_server_performance: reset_prng failed with " << ret << std::endl;
  } else {
    std::cout << "kv_server_performance: reset_prng succeeded." << std::endl;
  }

  std::cout << "kv_server_performance: requesting counter" << std::endl;
  uint32_t index;
  result = request_counter(enclave, &ret, &index);
  if (result != OE_OK) {
    ret = 1;
  }
  if (ret != 0) {
    std::cerr << "kv_server_performance: request_counter failed with " << ret << std::endl;
  } else {
    std::cout << "kv_server_performance: request_counter succeeded with index: " << index << std::endl;
  }
      

  auto performance_consens = std::make_unique<FidesConsensus>(
      *config, std::make_unique<KVExecutor>(std::make_unique<MemoryDB>()), enclave);

/* // For transaction decryption
  
  std::cout << "kv_server_performance: generate key" << std::endl;
  size_t key_size = 2048;
  result = generate_key(enclave, &ret, &key_size);
  std::cout << "After generate_key" << std::endl;
  if (result != OE_OK || ret != 0) {
    std::cerr << "kv_server_performance: generate_key failed with " << ret << std::endl;
    // goto exit;
  }
  
  result = get_pubkey(enclave, &ret, &key_buf, &key_len);

  bio = BIO_new_mem_buf(key_buf, -1);
  pub_key = NULL;
  PEM_read_bio_RSAPublicKey(bio, &pub_key, NULL, NULL);
  
  encrypted_len = RSA_size(pub_key);
  encrypted_data = new unsigned char[encrypted_len]; // Initialize buffer


{
  // KVRequest request;
  // request.set_cmd(KVRequest::SET);
  // request.set_key(GetEncryptedRandomKey());
  // request.set_value("helloworld");
  
  
  // int requestSize = request.ByteSizeLong();
  // unsigned char* binary_data = new unsigned char[requestSize];

  // if (request.SerializeToArray(binary_data, requestSize)) {
  //   std::cout << "Serialization successful!" << std::endl;
  // } else {
  //   std::cerr << "Serialization failed!" << std::endl;
  // }

  // std::cout << "kv_server_performance: encrypt data" << std::endl;
  // unsigned char* encrypted_data = nullptr;
  // size_t encrypted_len = 0;
  // result = encrypt(enclave, &ret, binary_data, 
  //                 &encrypted_data, requestSize,&encrypted_len);

  // printf("kv_server_performance: Finish encrypt data\n");
  // printf("encrypted_data: %s\n", encrypted_data);

  // std::string request_data(encrypted_data, encrypted_data + encrypted_len);
  

  KVRequest request;
  request.set_cmd(KVRequest::SET);
  request.set_key(GetRandomKey());
  request.set_value("helloworld");
  std::string request_data;
  request.SerializeToString(&request_data);
  
  const unsigned char* request_data_char = reinterpret_cast<const unsigned char*>(request_data.c_str());

  encrypted_data = new unsigned char[encrypted_len]; // Initialize buffer
  std::cout<<"Before RSA_public_encrypt.";
  size_t result_len = RSA_public_encrypt(request_data.size(), request_data_char, encrypted_data, 
                                         pub_key, RSA_PKCS1_PADDING);

  std::string encrypted_data_str(encrypted_data, encrypted_data + result_len);

  performance_consens->SetupPerformanceDataFunc([encrypted_data_str]() {
    return encrypted_data_str;
  });
}

  printf("Run 4\n");
*/

  performance_consens->SetupPerformanceDataFunc([]() {
    KVRequest request;
    request.set_cmd(KVRequest::SET);
    request.set_key(GetRandomKey());
    request.set_value("helloworld");
    std::string request_data;
    request.SerializeToString(&request_data);
    return request_data;
  });

  auto server =
      std::make_unique<ServiceNetwork>(*config,
      std::move(performance_consens));
  server->Run();

  BIO_free_all(bio);
  // delete[] binary_data;
}
