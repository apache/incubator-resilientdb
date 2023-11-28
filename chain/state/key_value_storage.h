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

#pragma once

#include <memory>
#include <list>
#include <map>
#include <unordered_map>

#include "chain/storage/storage.h"

namespace resdb {

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
class KeyValueStorage : public Storage {
 public:
  KeyValueStorage();

  int SetValue(const std::string& key, const std::string& value);
  std::string GetValue(const std::string& key);

  std::string GetAllValues() override;
  std::string GetRange(const std::string& min_key, const std::string& max_key) override;

  int SetValueWithVersion(const std::string& key, const std::string& value, int version) override;
  std::pair<std::string,int> GetValueWithVersion(const std::string& key, int version) override;

  // Return a map of <key, <value, version>>
  std::map<std::string,std::pair<std::string,int>> GetAllValuesWithVersion() override;
  std::map<std::string,std::pair<std::string,int>> GetRangeWithVersion(
    const std::string& min_key, const std::string& max_key) override;

  // Return a list of <value, version>
  std::vector<std::pair<std::string,int>> GetHistory(
    const std::string& key, const int& min_version, const int& max_version) override;

 private:
  std::unordered_map<std::string, std::string> kv_map_;
  std::unordered_map<std::string, std::list<std::pair<std::string, int>>> kv_map_with_v_;
};

}  // namespace resdb
