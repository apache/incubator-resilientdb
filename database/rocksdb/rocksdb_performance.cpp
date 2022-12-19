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
#include <glog/logging.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/write_batch.h"

using rocksdb::ReadOptions;
using rocksdb::WriteOptions;

using namespace std::chrono;

const int KEY_SIZE = 8;
const bool ENABLE_BATCH_WRITE = false;
const int BATCH_SIZE = 1000;

std::string GetRandomKey() {
  std::string key = "";
  for (int i = 0; i < KEY_SIZE; i++) {
    int num = std::rand() % 10;
    key += std::to_string(num);
  }
  return key;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("enter # of writes as a command line argument\n");
    return 0;
  }
  std::string temp_str(argv[1]);
  int num_pairs = std::stoi(temp_str);

  std::unique_ptr<rocksdb::DB> db = nullptr;
  rocksdb::Options options;

  options.IncreaseParallelism(8);
  options.OptimizeLevelStyleCompaction();
  options.write_buffer_size = 128 << 20;
  options.create_if_missing = true;

  rocksdb::DB* db_ = nullptr;
  rocksdb::Status status =
      rocksdb::DB::Open(options, "/tmp/myrocksdb-performance", &db_);

  if (status.ok()) db = std::unique_ptr<rocksdb::DB>(db_);

  WriteOptions woptions = WriteOptions();

  std::vector<std::string> key_nums;
  for (int i = 0; i < num_pairs; i++) {
    key_nums.push_back(GetRandomKey());
  }

  auto start = high_resolution_clock::now();

  if (!ENABLE_BATCH_WRITE) {
    for (int i = 0; i < num_pairs; i++) {
      status = db->Put(woptions, key_nums[i], "helloworld");
    }
  } else {
    for (int i = 0; i < num_pairs / BATCH_SIZE; i++) {
      rocksdb::WriteBatch batch;
      for (int j = 0; j < BATCH_SIZE; j++) {
        batch.Put(key_nums[i], "helloworld");
      }
      db->Write(woptions, &batch);
    }
  }

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);

  LOG(INFO) << duration.count() << " milliseconds for " << num_pairs
            << " writes";

  start = high_resolution_clock::now();
  rocksdb::Iterator* itr = db->NewIterator(ReadOptions());
  itr->SeekToFirst();
  while (itr->Valid()) {
    itr->Next();
  }
  delete itr;
  stop = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(stop - start);

  LOG(INFO) << duration.count() << " milliseconds for iterating";

  const int NUM_READ_SETS = 11;
  ReadOptions roptions = ReadOptions();
  for (int i = 0; i < NUM_READ_SETS; i++) {
    std::string value;
    start = high_resolution_clock::now();

    for (int j = 0; j < num_pairs / 10; j++) {
      int key_index = std::rand() % num_pairs;
      std::string key = key_nums[key_index];
      status = db->Get(roptions, key, &value);
    }

    stop = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(stop - start);

    LOG(INFO) << duration.count() << " milliseconds for " << num_pairs / 10
              << " reads (set #" << i << ")";
  }
  db.reset();
}
