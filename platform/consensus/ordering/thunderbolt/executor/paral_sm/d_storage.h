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

#include <map>
#include <shared_mutex>

#include "eEVM/util.h"
#include "platform/consensus/ordering/thunderbolt/executor/paral_sm/data_storage.h"

namespace resdb {
namespace contract {

class D_Storage : public DataStorage {
 public:
  virtual int64_t Store(const uint256_t& key, const uint256_t& value,
                        bool is_local);
  virtual int64_t StoreWithVersion(const uint256_t& key, const uint256_t& value,
                                   int version, bool is_local);
  virtual std::pair<uint256_t, int64_t> Load(const uint256_t& key,
                                             bool is_from_local_view) const;
  virtual bool Remove(const uint256_t& key, bool is_local);
  virtual bool Exist(const uint256_t& key, bool is_local) const;

  virtual int64_t GetVersion(const uint256_t& key, bool is_local) const;

  virtual void Reset(const uint256_t& key, const uint256_t& value,
                     int64_t version, bool is_local);

 protected:
  std::map<uint256_t, std::pair<uint256_t, int64_t> > c_s_;
  mutable std::shared_mutex mutex_;

  std::map<uint256_t, std::pair<uint256_t, int64_t> > g_s_;
  mutable std::shared_mutex g_mutex_;
};

}  // namespace contract
}  // namespace resdb
