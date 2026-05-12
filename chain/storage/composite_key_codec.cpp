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

 #include "chain/storage/composite_key_codec.h"

 #include <cstring>
 
 namespace resdb {
 namespace storage {
 
std::string EncodeCompositeKey(const std::string& index_name,
                               const std::vector<std::string>& attributes,
                               const std::string& primary_key) {
  if (index_name.find(kCompositeKeyDelim) != std::string::npos ||
      primary_key.find(kCompositeKeyDelim) != std::string::npos) {
    return "";
  }
  for (const auto& attr : attributes) {
    if (attr.find(kCompositeKeyDelim) != std::string::npos) {
      return "";
    }
  }

  std::string result;
  result.append(kCompositeKeyNamespace);
  result.push_back(kCompositeKeyDelim);
  result.append(index_name);
  result.push_back(kCompositeKeyDelim);

  for (const auto& attr : attributes) {
    result.append(attr);
    result.push_back(kCompositeKeyDelim);
  }

  result.append(primary_key);
  return result;
}
 
std::string EncodeCompositeKeyPrefix(
    const std::string& index_name,
    const std::vector<std::string>& attribute_prefix) {
  if (index_name.find(kCompositeKeyDelim) != std::string::npos) {
    return "";
  }
  for (const auto& attr : attribute_prefix) {
    if (attr.find(kCompositeKeyDelim) != std::string::npos) {
      return "";
    }
  }

  std::string result;
  result.append(kCompositeKeyNamespace);
  result.push_back(kCompositeKeyDelim);
  result.append(index_name);
  result.push_back(kCompositeKeyDelim);

  for (size_t i = 0; i < attribute_prefix.size(); ++i) {
    if (i > 0) result.push_back(kCompositeKeyDelim);
    result.append(attribute_prefix[i]);
  }

  // Trailing delimiter makes this a strict byte prefix of the full key.
  result.push_back(kCompositeKeyDelim);
  return result;
}

bool DecodeCompositeKey(const std::string& encoded,
                        std::string* index_name,
                        std::vector<std::string>* attributes,
                        std::string* primary_key) {
  if (!index_name || !attributes || !primary_key) {
    return false;
  }

  size_t ns_len = std::strlen(kCompositeKeyNamespace);
  if (encoded.size() < ns_len + 1 ||
      encoded.compare(0, ns_len, kCompositeKeyNamespace) != 0) {
    return false;
  }

  std::string rest = encoded.substr(ns_len + 1);
  size_t pos = 0;
  std::vector<std::string> parts;
  while (pos < rest.size()) {
    size_t next = rest.find(kCompositeKeyDelim, pos);
    if (next == std::string::npos) {
      parts.push_back(rest.substr(pos));
      break;
    }
    parts.push_back(rest.substr(pos, next - pos));
    pos = next + 1;
  }

  // Need at least index_name and primary_key.
  if (parts.size() < 2) {
    return false;
  }

  *index_name = parts[0];
  *primary_key = parts.back();
  attributes->clear();
  for (size_t i = 1; i < parts.size() - 1; ++i) {
    attributes->push_back(parts[i]);
  }
  return true;
}
 
 }  // namespace storage
 }  // namespace resdb