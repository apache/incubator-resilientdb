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

#include "chain/storage/storage.h"
#include "gmock/gmock.h"

namespace resdb {

class MockStorage : public Storage {
 public:
  MOCK_METHOD(int, SetValue, (const std::string& key, const std::string& value),
              (override));
  MOCK_METHOD(std::string, GetValue, (const std::string& key), (override));
  MOCK_METHOD(std::string, GetAllValues, (), (override));
  MOCK_METHOD(std::string, GetRange, (const std::string&, const std::string&),
              (override));

  MOCK_METHOD(int, SetValueWithVersion,
              (const std::string& key, const std::string& value, int version),
              (override));

  using ValueType = std::pair<std::string, int>;
  using ItemsType = std::map<std::string, ValueType>;
  using ValuesType = std::vector<ValueType>;

  MOCK_METHOD(ValueType, GetValueWithVersion,
              (const std::string& key, int version), (override));
  MOCK_METHOD(ItemsType, GetAllItems, (), (override));
  MOCK_METHOD(ItemsType, GetKeyRange, (const std::string&, const std::string&),
              (override));
  MOCK_METHOD(ValuesType, GetHistory, (const std::string&, int, int),
              (override));
  MOCK_METHOD(ValuesType, GetTopHistory, (const std::string&, int), (override));

  MOCK_METHOD(bool, Flush, (), (override));
};

}  // namespace resdb
