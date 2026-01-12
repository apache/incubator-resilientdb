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

#include "eEVM/globalstate.h"

namespace resdb {
namespace contract {

class EVMState : public eevm::GlobalState {
 public:
  EVMState() = default;
  virtual ~EVMState() = default;

 protected:
  const eevm::Block& get_current_block() override { return block_; }
  uint256_t get_block_hash(uint8_t offset) override { return 0; }

 private:
  // Unused.
  eevm::Block block_;
};

}  // namespace contract
}  // namespace resdb
