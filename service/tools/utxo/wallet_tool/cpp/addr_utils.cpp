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

#include "service/tools/utxo/wallet_tool/cpp/addr_utils.h"

#include <glog/logging.h>

#include "common/crypto/hash.h"

namespace resdb {
namespace coin {
namespace utils {

using resdb::utils::CalculateRIPEMD160Hash;
using resdb::utils::CalculateSHA256Hash;

std::string GenAddr(const std::string& public_key) {
  std::string sha256_hash = CalculateSHA256Hash(public_key);
  std::string ripemd160_hash = CalculateRIPEMD160Hash(sha256_hash);
  return ripemd160_hash;
}

}  // namespace utils
}  // namespace coin
}  // namespace resdb
