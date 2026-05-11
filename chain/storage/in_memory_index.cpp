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

#include "chain/storage/in_memory_index.h"

namespace resdb {
namespace storage {

void InMemoryHashIndex::Add(const std::string& key, const std::string& cid) {
  index_[key] = cid;
}

std::string InMemoryHashIndex::Get(const std::string& key) const {
  auto it = index_.find(key);
  if (it != index_.end()) {
    return it->second;
  }
  return "";
}

void InMemoryHashIndex::Clear() {
  index_.clear();
}

size_t InMemoryHashIndex::Size() const {
  return index_.size();
}

}  // namespace storage
}  // namespace resdb
