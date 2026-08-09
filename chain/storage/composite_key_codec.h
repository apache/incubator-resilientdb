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

#include <string>
#include <vector>

namespace resdb {
namespace storage {

// Reserved prefix for index entries; keeps them out of the user key space.
constexpr char kCompositeKeyNamespace[] = "ck";

// Field separator. Null byte is illegal in user attributes, so it is safe.
constexpr char kCompositeKeyDelim = '\0';

// Layout: "ck" \0 index_name \0 attr_1 \0 ... \0 primary_key
// Returns "" if any input contains the delimiter.
std::string EncodeCompositeKey(const std::string& index_name,
                               const std::vector<std::string>& attributes,
                               const std::string& primary_key);

// Same layout without primary_key; ends with a trailing delimiter so the
// result is a strict byte prefix of the corresponding full key.
std::string EncodeCompositeKeyPrefix(
    const std::string& index_name,
    const std::vector<std::string>& attribute_prefix);

// Returns false if input is malformed.
bool DecodeCompositeKey(const std::string& encoded,
                        std::string* index_name,
                        std::vector<std::string>* attributes,
                        std::string* primary_key);

}  // namespace storage
}  // namespace resdb
