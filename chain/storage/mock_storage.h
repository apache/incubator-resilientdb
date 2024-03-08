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
