#include <glog/logging.h>

#include <iostream>
#include <sstream>
#include <string>

#include "leveldb/db.h"

int main(int argc, char** argv) {
  // Set up database connection information and open database
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;

  leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);

  if (false == status.ok()) {
    LOG(ERROR) << "Unable to open/create test database './testdb'. status:"
               << status.ToString();
    return -1;
  }

  // Add 256 values to the database
  leveldb::WriteOptions writeOptions;
  for (unsigned int i = 0; i < 256; ++i) {
    std::ostringstream keyStream;
    keyStream << "Key" << i;

    std::ostringstream valueStream;
    valueStream << "Test data value: " << i;

    db->Put(writeOptions, keyStream.str(), valueStream.str());
  }

  // Iterate over each item in the database and print them
  leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    LOG(ERROR) << " iterator key:" << it->key().ToString()
               << " value:" << it->value().ToString();
  }

  if (false == it->status().ok()) {
    LOG(ERROR) << "An error was found during the scan. status:"
               << it->status().ToString();
  }

  delete it;

  // Close the database
  delete db;
}
