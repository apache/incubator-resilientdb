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

#include "common/lru/lru_cache.h"

#include <gtest/gtest.h>

namespace resdb {
namespace {

class LRUCacheTest : public ::testing::Test {
 protected:
  LRUCache<int, int> cache_{3};
};

TEST_F(LRUCacheTest, TestPutAndGet) {
  cache_.Put(1, 100);
  EXPECT_EQ(cache_.Get(1), 100);
  EXPECT_EQ(cache_.GetCacheHits(), 1);
  EXPECT_EQ(cache_.GetCacheMisses(), 0);
}

TEST_F(LRUCacheTest, TestUpdateValue) {
  cache_.Put(1, 100);
  cache_.Put(1, 200);
  EXPECT_EQ(cache_.Get(1), 200);
  EXPECT_EQ(cache_.GetCacheHits(), 1);
  EXPECT_EQ(cache_.GetCacheMisses(), 0);
}

TEST_F(LRUCacheTest, TestEviction) {
  cache_.Put(1, 100);
  cache_.Put(2, 200);
  cache_.Put(3, 300);
  cache_.Put(4, 400);  // This should evict key 1

  EXPECT_EQ(cache_.Get(1), 0);  // Key 1 should be evicted
  EXPECT_EQ(cache_.Get(2), 200);
  EXPECT_EQ(cache_.Get(3), 300);
  EXPECT_EQ(cache_.Get(4), 400);

  EXPECT_EQ(cache_.GetCacheHits(), 3);
  EXPECT_EQ(cache_.GetCacheMisses(), 1);
}

TEST_F(LRUCacheTest, TestUsageOrder) {
  cache_.Put(1, 100);
  cache_.Put(2, 200);
  cache_.Put(3, 300);

  // Access key 1 to update its usage order
  EXPECT_EQ(cache_.Get(1), 100);

  // Add a new key, which should evict key 2 (least recently used)
  cache_.Put(4, 400);

  EXPECT_EQ(cache_.Get(1), 100);
  EXPECT_EQ(cache_.Get(2), 0);  // Key 2 should be evicted
  EXPECT_EQ(cache_.Get(3), 300);
  EXPECT_EQ(cache_.Get(4), 400);

  EXPECT_EQ(cache_.GetCacheHits(), 4);
  EXPECT_EQ(cache_.GetCacheMisses(), 1);
}

TEST_F(LRUCacheTest, TestFlush) {
  cache_.Put(1, 100);
  cache_.Put(2, 200);
  cache_.Flush();

  EXPECT_EQ(cache_.Get(1), 0);
  EXPECT_EQ(cache_.Get(2), 0);
  EXPECT_EQ(cache_.GetCacheHits(), 0);
  EXPECT_EQ(cache_.GetCacheMisses(), 2);
}

}  // namespace
}  // namespace resdb
