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

#include "service/tools/utxo/wallet_tool/cpp/key_utils.h"

#include "common/crypto/key_generator.h"

namespace resdb {
namespace coin {
namespace utils {

std::pair<std::string, std::string> GenECDSAKeys() {
  resdb::SecretKey key =
      resdb::KeyGenerator::GeneratorKeys(resdb::SignatureInfo::ECDSA);
  return std::make_pair(key.private_key(), key.public_key());
}

}  // namespace utils
}  // namespace coin
}  // namespace resdb
