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
