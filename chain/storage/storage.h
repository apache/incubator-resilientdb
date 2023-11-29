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
