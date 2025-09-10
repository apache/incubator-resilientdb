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

#include <glog/logging.h>
#include <pybind11/pybind11.h>

#include "common/crypto/signature_utils.h"

namespace resdb {
namespace coin {

using utils::ECDSASignString;
using utils::ECDSAVerifyString;

bool TestKeyPairs(std::string private_key, std::string public_key) {
  LOG(ERROR) << " private key:" << private_key << " public key:" << public_key;
  std::string message = "hello world";
  std::string sign = ECDSASignString(private_key, message);
  LOG(ERROR) << "sign done:" << sign;
  bool res = ECDSAVerifyString(message, public_key, sign);
  LOG(ERROR) << "verify done. res:" << (res == true);
  return res;
}

}  // namespace coin
}  // namespace resdb

PYBIND11_MODULE(key_tester_utils, m) {
  m.doc() = "ECDSA Key Tester";
  m.def("TestKeyPairs", &resdb::coin::TestKeyPairs, "Test a ecdsa key pair.");
}
