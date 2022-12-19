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

#include "benchmark/pbft/benchmark_client.h"
#include "common/utils/utils.h"
#include "config/resdb_config_utils.h"

using resdb::BenchmarkClient;
using resdb::GenerateResDBConfig;
using resdb::get_sys_clock;
using resdb::ReplicaInfo;
using resdb::ResDBConfig;

int main(int argc, char** argv) {
  if (argc < 5) {
    printf("<config path> <data_size> <request num> <thread num> \n");
    return 0;
  }
  std::string config_file = argv[1];
  int value_size = std::stoi(argv[2]);

  ResDBConfig config = GenerateResDBConfig(config_file);

  config.SetClientTimeoutMs(10000000);
  BenchmarkClient client(config);
  int ret = client.Set(std::string(value_size, 't'));
  if (ret != 0) {
    printf("client set fail ret = %d\n", ret);
  }
}
