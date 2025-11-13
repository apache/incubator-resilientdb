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

#include "chain/storage/prefix_allocator.h"

#include "absl/status/status.h"
#include "gtest/gtest.h"

namespace resdb {
namespace storage {
namespace {

TEST(PrefixAllocatorTest, ReserveAndFetch) {
  auto& allocator = PrefixAllocator::Instance();
  constexpr char kOwner[] = "test_owner_reserve_fetch";
  EXPECT_TRUE(allocator.Reserve(kOwner, "test_prefix/").ok());

  auto status_or_prefix = allocator.GetPrefix(kOwner);
  ASSERT_TRUE(status_or_prefix.ok());
  EXPECT_EQ(*status_or_prefix, "test_prefix/");

  auto duplicate = allocator.Reserve(kOwner, "test_prefix/");
  EXPECT_TRUE(duplicate.ok());
}

TEST(PrefixAllocatorTest, DetectsConflicts) {
  auto& allocator = PrefixAllocator::Instance();
  EXPECT_TRUE(allocator.Reserve("conflict_owner_a", "conflict_prefix/").ok());
  auto conflict = allocator.Reserve("conflict_owner_b", "conflict_prefix/");
  EXPECT_EQ(conflict.code(), absl::StatusCode::kAlreadyExists);
}

TEST(PrefixAllocatorTest, MissingOwnerReturnsNotFound) {
  auto& allocator = PrefixAllocator::Instance();
  auto missing = allocator.GetPrefix("missing_owner");
  EXPECT_EQ(missing.status().code(), absl::StatusCode::kNotFound);
}

}  // namespace
}  // namespace storage
}  // namespace resdb
