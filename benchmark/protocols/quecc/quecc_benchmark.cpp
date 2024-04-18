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

#include <chrono>
#include <cstdint>
#include <ctime>

#include "chain/storage/memory_db.h"
#include "executor/kv/kv_executor.h"
#include "executor/kv/quecc_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"

using resdb::BatchUserRequest;
using resdb::BatchUserResponse;
using resdb::GenerateResDBConfig;
using resdb::KVExecutor;
using resdb::KVOperation;
using resdb::KVRequest;
using resdb::storage::MemoryDB;
using resdb::QueccExecutor;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::Storage;
void ShowUsage() {
  printf(
      "<config> <private_key> <cert_file> <durability_option> [logging_dir]\n");
}

BatchUserRequest EqualDistribution() {
  BatchUserRequest batch_request;

  vector<string> keys = {"test1", "test2", "test3", "test4", "test5"};

  for (int i = 0; i < 10000; i++) {
    // add transaction
    KVRequest request;

    for (int j = 0; j < 100; j++) {
      // add operation
      resdb::Operation* op = request.add_ops();
      op->set_cmd(resdb::Operation::SET);
      op->set_key(keys[i % 5]);
      op->set_value(to_string(j));
    }

    std::string req_str;
    if (!request.SerializeToString(&req_str)) {
      exit(0);
    }
    batch_request.add_user_requests()->mutable_request()->set_data(req_str);
  }

  return batch_request;
}

BatchUserRequest NoDistribution() {
  BatchUserRequest batch_request;

  for (int i = 0; i < 10000; i++) {
    KVRequest request;

    for (int j = 0; j < 100; j++) {
      // add operation
      resdb::Operation* op = request.add_ops();
      op->set_cmd(resdb::Operation::SET);
      op->set_key("test6");
      op->set_value(to_string(j));
    }

    std::string str;
    if (!request.SerializeToString(&str)) {
      exit(0);
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }
  return batch_request;
}

BatchUserRequest RandomDistribution() {
  BatchUserRequest batch_request;
  srand(time(NULL));
  vector<string> keys = {"test7", "test8", "test9", "test10", "test11"};

  for (int i = 0; i < 10000; i++) {
    KVRequest request;

    for (int j = 0; j < 100; j++) {
      // add operation
      resdb::Operation* op = request.add_ops();
      op->set_cmd(resdb::Operation::SET);
      op->set_key(keys[rand() % 5]);
      op->set_value(to_string(j));
    }

    std::string str;
    if (!request.SerializeToString(&str)) {
      exit(0);
    }
    batch_request.add_user_requests()->mutable_request()->set_data(str);
  }
  return batch_request;
}

int main(int argc, char** argv) {
  vector<BatchUserRequest> equal_split_array;
  vector<BatchUserRequest> no_split_array;
  vector<BatchUserRequest> random_split_array;
  for (int i = 0; i < 1; i++) {
    equal_split_array.push_back(EqualDistribution());
    // no_split_array.push_back(NoDistribution());
    // random_split_array.push_back(RandomDistribution());
  }
  KVExecutor kv_executor=KVExecutor(std::make_unique<MemoryDB>());

  QueccExecutor quecc_executor=QueccExecutor(std::make_unique<MemoryDB>());

  std::unique_ptr<BatchUserResponse> response;
  // Equal Split Comparison
  printf("Equal Split Times\n");

  auto start_time = std::chrono::high_resolution_clock::now();
  for (BatchUserRequest equal_split : equal_split_array) {
    response = quecc_executor.ExecuteBatch(equal_split);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);
  printf("Quecc Time Taken: %d\n", (int)duration.count());
  start_time = std::chrono::high_resolution_clock::now();

  for (BatchUserRequest equal_split : equal_split_array) {
    response = kv_executor.ExecuteBatch(equal_split);
  }
  end_time = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time -
                                                                   start_time);
  printf("KV Time Taken: %d\n", (int)duration.count());
}