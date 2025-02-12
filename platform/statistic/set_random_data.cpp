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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string Exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;

  auto pipe = popen(cmd, "r");  // get rid of shared_ptr

  if (!pipe) throw std::runtime_error("popen() failed!");

  while (!feof(pipe)) {
    if (fgets(buffer.data(), 128, pipe) != nullptr) result += buffer.data();
  }

  auto rc = pclose(pipe);

  if (rc == EXIT_SUCCESS) {  // == 0

  } else if (rc == EXIT_FAILURE) {  // EXIT_FAILURE is not used by all programs,
                                    // maybe needs some adaptation.
  }
  return result;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("<test/loop> <value> \n");
    return 0;
  }
  std::string command = argv[1];
  std::string value = argv[2];
  std::string output = "";
  int count = 0;

  if (command == "test") {
    for (int i = 0; i < std::stoi(value); i++) {
      std::stringstream ss;
      ss << " bazel-bin/service/tools/kv/api_tools/kv_service_tools --config "
            "service/tools/config/interface/service.config --cmd set "
         << "--key key" << (std::rand() % 500) << " " << "--value value"
         << (std::rand() % 500);

      std::string test = ss.str();
      output = Exec(test.c_str());
      std::cout << i << " " << output << std::endl;
    }
  } else if (command == "loop") {
    while (1) {
      std::string test =
          "/home/jyu25/nexres/bazel-bin/example/kv_server_tools "
          "/home/jyu25/nexres/example/kv_client_config.config set " +
          std::to_string(std::rand() % 500) + " " +
          std::to_string(std::rand() % 500);
      output = Exec(test.c_str());
      std::cout << ++count << " " << output << std::endl;
    }
  }

  return 0;
}