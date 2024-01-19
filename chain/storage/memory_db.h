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

#pragma once

#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "chain/storage/storage.h"

namespace resdb {
namespace storage {

// Key Value Storage supporting two types of interfaces:
// Non-version:
//  It provides set and get function to set a value to a specific key
//  Values will be set directly.
// Version:
//  It provides set and get function to set a value to a specific key
//  with a version.
//  The version inside setting function is to support OCC verification.
//  The version value should be obtained before setting
//  and returned back when setting a new value. If the version is not
//  the same as the old one, return failure.
//  If the value of a specific version does not exist or providing
//  version 0 as parameter when accessing get
//  it returns the current value along its version.
//
// Note: Only one type of interface are allowed to be used.
//

std::unique_ptr<Storage> NewMemoryDB();

class MemoryDB : public Storage {
 public:
  MemoryDB();

  int SetValue(const std::string& key, const std::string& value);
  std::string GetValue(const std::string& key);

  std::string GetAllValues() override;
  std::string GetRange(const std::string& min_key,
                       const std::string& max_key) override;

  int SetValueWithVersion(const std::string& key, const std::string& value,
                          int version) override;
  std::pair<std::string, int> GetValueWithVersion(const std::string& key,
                                                  int version) override;

  // Return a map of <key, <value, version>>
  std::map<std::string, std::pair<std::string, int>> GetAllItems() override;
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) override;

  // Return a list of <value, version>
  std::vector<std::pair<std::string, int>> GetHistory(const std::string& key,
                                                      int min_version,
                                                      int max_version) override;

  std::vector<std::pair<std::string, int>> GetTopHistory(const std::string& key,
                                                         int number) override;

 private:
  std::unordered_map<std::string, std::string> kv_map_;
  std::unordered_map<std::string, std::list<std::pair<std::string, int>>>
      kv_map_with_v_;
};

}  // namespace storage
}  // namespace resdb
