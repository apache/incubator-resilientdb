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

#include <gtest/gtest.h>

#include "chain/storage/composite_key_codec.h"

using namespace resdb::storage;

TEST(CompositeKeyCodecTest, RoundTrip) {
  std::string encoded =
      EncodeCompositeKey("byOwner", {"alice", "active"}, "user:1");
  EXPECT_FALSE(encoded.empty());

  std::string idx, pk;
  std::vector<std::string> attrs;
  ASSERT_TRUE(DecodeCompositeKey(encoded, &idx, &attrs, &pk));
  EXPECT_EQ(idx, "byOwner");
  EXPECT_EQ(attrs.size(), 2u);
  EXPECT_EQ(attrs[0], "alice");
  EXPECT_EQ(attrs[1], "active");
  EXPECT_EQ(pk, "user:1");
}

TEST(CompositeKeyCodecTest, RejectDelimInInput) {
  // C-string literals truncate at '\0'; build the strings explicitly.
  std::string bad_index = std::string("in") + kCompositeKeyDelim + "dex";
  EXPECT_TRUE(EncodeCompositeKey(bad_index, {"attr"}, "pk").empty());

  std::string bad_attr = std::string("at") + kCompositeKeyDelim + "tr";
  EXPECT_TRUE(EncodeCompositeKey("idx", {bad_attr}, "pk").empty());

  std::string bad_pk = std::string("p") + kCompositeKeyDelim + "k";
  EXPECT_TRUE(EncodeCompositeKey("idx", {"attr"}, bad_pk).empty());
}

TEST(CompositeKeyCodecTest, PrefixIsStrictBytePrefix) {
  std::string full = EncodeCompositeKey("byOwner", {"alice"}, "user:1");
  std::string prefix = EncodeCompositeKeyPrefix("byOwner", {"alice"});

  EXPECT_FALSE(full.empty());
  EXPECT_FALSE(prefix.empty());
  EXPECT_EQ(full.find(prefix), 0u);
  EXPECT_LT(prefix.size(), full.size());
}

TEST(CompositeKeyCodecTest, DecodeMalformed) {
  std::string idx, pk;
  std::vector<std::string> attrs;

  std::string no_ns = std::string("no_prefix") + kCompositeKeyDelim + "key";
  EXPECT_FALSE(DecodeCompositeKey(no_ns, &idx, &attrs, &pk));

  std::string only_one = std::string("ck") + kCompositeKeyDelim + "alone";
  EXPECT_FALSE(DecodeCompositeKey(only_one, &idx, &attrs, &pk));
}

TEST(CompositeKeyCodecTest, EmptyAttributes) {
  std::string encoded = EncodeCompositeKey("byId", {}, "user:1");
  EXPECT_FALSE(encoded.empty());

  std::string idx, pk;
  std::vector<std::string> attrs;
  ASSERT_TRUE(DecodeCompositeKey(encoded, &idx, &attrs, &pk));
  EXPECT_EQ(idx, "byId");
  EXPECT_EQ(attrs.size(), 0u);
  EXPECT_EQ(pk, "user:1");
}
