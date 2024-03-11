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

#include "gmock/gmock.h"
#include "service/contract/executor/manager/data_storage.h"

namespace resdb {
namespace contract {

class MockStorage : public DataStorage {
 public:
 typedef std::pair<uint256_t, int64_t> LoadType;

  MOCK_METHOD(int64_t, Store, (const uint256_t& key, const uint256_t& value, bool), (override));
  MOCK_METHOD(bool, Remove, (const uint256_t&, bool), (override));
  MOCK_METHOD(bool, Exist, (const uint256_t&, bool), (const, override));
  MOCK_METHOD(int64_t, GetVersion, (const uint256_t&, bool), (const, override));
  MOCK_METHOD(LoadType, Load, (const uint256_t&, bool), (const, override));
};

}
}  // namespace resdb
