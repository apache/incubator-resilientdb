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
#include <benchmark/benchmark.h>
#include "executor/kv/kv_executor.h"
#include "executor/kv/quecc_executor.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/proto/resdb.pb.h"
#include "proto/kv/kv.pb.h"

using resdb::BatchUserRequest;
using resdb::BatchUserResponse;
using resdb::GenerateResDBConfig;
using resdb::KVExecutor;
using resdb::Operation;
using resdb::KVRequest;
using resdb::QueccExecutor;
using resdb::ResConfigData;
using resdb::ResDBConfig;
using resdb::Storage;

using namespace resdb::storage;

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

    for (int j = 0; j < 10; j++) {
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

static void BM_EqualDistribution(benchmark::State& state) {
	std::unique_ptr<Storage> storage1 = nullptr;
	storage1 = NewMemoryDB();
	std::unique_ptr<BatchUserResponse> response;
	vector<BatchUserRequest> equal_split_array;
	QueccExecutor quecc_executor(std::move(storage1));
	
	for (int i = 0; i < 10; i++) {
		equal_split_array.push_back(EqualDistribution());
	}

	for (auto _ : state) {
		for (BatchUserRequest equal_split : equal_split_array) {
    	// response = quecc_executor.ExecuteBatch(equal_split);
			// ::benchmark::DoNotOptimize(response);
  	}
	}

	if (state.thread_index() == 0) {
		double mean = state.counters["mean"];
		benchmark::DoNotOptimize(mean);
	}
}

static void BM_NoDistribution(benchmark::State& state) {
	std::unique_ptr<Storage> storage1 = nullptr;
	storage1 = NewMemoryDB();
	std::unique_ptr<BatchUserResponse> response;
	vector<BatchUserRequest> no_split_array;
	QueccExecutor quecc_executor(std::move(storage1));
	
	for (int i = 0; i < 10; i++) {
		no_split_array.push_back(NoDistribution());
	}

	for (auto _ : state) {
		for (BatchUserRequest no_split : no_split_array) {
    	response = quecc_executor.ExecuteBatch(no_split);
			::benchmark::DoNotOptimize(response);
  	}
	}
}

static void BM_RandomDistribution(benchmark::State& state) {
	std::unique_ptr<Storage> storage1 = nullptr;
	storage1 = NewMemoryDB();
	std::unique_ptr<BatchUserResponse> response;
	vector<BatchUserRequest> random_split_array;
	QueccExecutor quecc_executor(std::move(storage1));
	
	for (int i = 0; i < 10; i++) {
		random_split_array.push_back(RandomDistribution());
	}

	for (auto _ : state) {
		for (BatchUserRequest rand_split : random_split_array) {
    	response = quecc_executor.ExecuteBatch(rand_split);
			::benchmark::DoNotOptimize(response);
  	}
	}
}

BENCHMARK(BM_EqualDistribution)->Name("Equal Split - Quecc")
	->Unit(benchmark::kSecond)->Iterations(10);

// BENCHMARK(BM_NoDistribution)->Name("No Split - Quecc")
// 	->Unit(benchmark::kSecond);

// BENCHMARK(BM_RandomDistribution)->Name("Random Split - Quecc")
// 	->Unit(benchmark::kSecond);

int main(int argc, char** argv) {
	::benchmark::Initialize(&argc, argv);
	::benchmark::RunSpecifiedBenchmarks();
	::benchmark::Shutdown();
}