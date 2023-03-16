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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
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
      usleep(1000000);
      std::string test =
          "/home/jyu25/nexres/bazel-bin/example/kv_server_tools "
          "/home/jyu25/nexres/example/kv_client_config.config set " +
          std::to_string(std::rand() % 500) + " " +
          std::to_string(std::rand() % 500);
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
}