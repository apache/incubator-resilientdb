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

#include "chain/storage/memory_db.h"

#include <gtest/gtest.h>

namespace resdb {
namespace storage {

TEST(MemoryDBTest, Clear) {
  MemoryDB db;

  ASSERT_EQ(db.SetValue("key1", "value1"), 0);
  ASSERT_EQ(db.SetValueWithVersion("key2", "value2", 0), 0);
  ASSERT_EQ(db.SetValueWithSeq("key3", "value3", 1), 0);
  ASSERT_EQ(db.CreateCompositeKey("index:key1"), 0);

  EXPECT_EQ(db.GetValue("key1"), "value1");

  auto version_value = db.GetValueWithVersion("key2", 1);
  EXPECT_EQ(version_value.first, "value2");
  EXPECT_EQ(version_value.second, 1);

  auto seq_value = db.GetValueWithSeq("key3", 1);
  EXPECT_EQ(seq_value.first, "value3");
  EXPECT_EQ(seq_value.second, 1);

  EXPECT_EQ(db.GetByCompositeKeyPrefix("index:").size(), 1);

  db.Clear();

  EXPECT_EQ(db.GetValue("key1"), "");

  version_value = db.GetValueWithVersion("key2", 0);
  EXPECT_EQ(version_value.first, "");
  EXPECT_EQ(version_value.second, 0);

  seq_value = db.GetValueWithSeq("key3", 0);
  EXPECT_EQ(seq_value.first, "");
  EXPECT_EQ(seq_value.second, 0);

  EXPECT_TRUE(db.GetByCompositeKeyPrefix("index:").empty());
}

}  // namespace storage
}  // namespace resdb
