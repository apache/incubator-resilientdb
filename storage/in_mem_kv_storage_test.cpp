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

#include "storage/in_mem_kv_storage.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace resdb {
namespace {

TEST(KVServerExecutorTest, SetValue) {
  InMemKVStorage storage;

  EXPECT_EQ(storage.GetAllValues(), "[]");
  EXPECT_EQ(storage.SetValue("test_key", "test_value"), 0);
  EXPECT_EQ(storage.GetValue("test_key"), "test_value");

  // GetValues and GetRange may be out of order for in-memory, so we test up to
  // 1 key-value pair
  EXPECT_EQ(storage.GetAllValues(), "[test_value]");
  EXPECT_EQ(storage.GetRange("a", "z"), "[test_value]");
}

TEST(KVServerExecutorTest, GetValue) {
  InMemKVStorage storage;
  EXPECT_EQ(storage.GetValue("test_key"), "");
}

}  // namespace

}  // namespace resdb
