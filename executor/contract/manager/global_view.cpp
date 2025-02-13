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


#include "executor/contract/manager/global_view.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {

GlobalView::GlobalView(resdb::Storage* storage) : storage_(storage) {}

void GlobalView::store(const uint256_t& key, const uint256_t& value) {
  storage_->SetValue(eevm::to_hex_string(key), eevm::to_hex_string(value));
}

uint256_t GlobalView::load(const uint256_t& key) {
  return eevm::to_uint256(storage_->GetValue(eevm::to_hex_string(key)));
}

bool GlobalView::remove(const uint256_t& key) { return true; }

}  // namespace contract
}  // namespace resdb
