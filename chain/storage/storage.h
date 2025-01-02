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

#include <map>
#include <string>
#include <vector>

namespace resdb {

class Storage {
 public:
  Storage() = default;
  virtual ~Storage() = default;

  virtual int SetValue(const std::string& key, const std::string& value) = 0;
  virtual std::string GetValue(const std::string& key) = 0;
  virtual std::string GetAllValues() = 0;
  virtual std::string GetRange(const std::string& min_key,
                               const std::string& max_key) = 0;

  virtual int SetValueWithVersion(const std::string& key,
                                  const std::string& value, int version) = 0;
  virtual std::pair<std::string, int> GetValueWithVersion(
      const std::string& key, int version) = 0;

  // Return a map of <key, <value, version>>
  virtual std::map<std::string, std::pair<std::string, int>> GetAllItems() = 0;
  virtual std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) = 0;

  // Return a list of <value, version> from a key
  // The version list is sorted by the version value in descending order
  virtual std::vector<std::pair<std::string, int>> GetHistory(
      const std::string& key, int min_version, int max_version) = 0;

  virtual std::vector<std::pair<std::string, int>> GetTopHistory(
      const std::string& key, int number) = 0;

  virtual bool Flush() { return true; };
};

}  // namespace resdb
