#include <fcntl.h>
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

  printf("%ld milliseconds for %d writes\n", duration.count(), num_pairs);

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

    printf("%ld milliseconds for %d reads (set #%d)\n", duration.count(),
           num_pairs / 10, i);
  }
  db.reset();
}
