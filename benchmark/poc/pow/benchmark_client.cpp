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

#include "application/poc/pbft_client.h"
#include "config/resdb_config_utils.h"

using resdb::GenerateReplicaInfo;
using resdb::PBFTClient;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;

int main(int argc, char** argv) {
  if (argc < 5) {
    printf(
        "<config path> <private key path> <cert_file> <data_size> <request "
        "num> <thread num> <client_ip>\n");
    return 0;
  }
  std::string config_file = argv[1];
  std::string private_key_file = argv[2];
  std::string cert_file = argv[3];
  int value_size = std::stoi(argv[4]);
  int req_num = std::stoi(argv[5]);
  std::string client_ip = argv[6];

  ReplicaInfo self_info = GenerateReplicaInfo(0, client_ip, 88888);

  std::unique_ptr<ResDBConfig> config =
      GenerateResDBConfig(config_file, private_key_file, cert_file, self_info);

  srand(time(0));
  PBFTClient client(*config);
  for (int j = 0; j < req_num; ++j) {
    std::string value;
    for (int i = 0; i < value_size; ++i) {
      int idx = rand() % 52;
      if (idx >= 26) {
        value += 'A' + (idx - 26);
      } else {
        value += 'a' + (idx - 26);
      }
    }
    int ret = client.Set(value);
    if (ret != 0) {
      printf("client set fail ret = %d\n", ret);
    }
    usleep(100);
    //	printf("send ret = %d len = %d\n",ret,value.size());
  }
  sleep(5);
  std::cout << " num??:" << req_num << " done";
}
