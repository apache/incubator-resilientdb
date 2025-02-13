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

#include <pybind11/pybind11.h>

#include "service/tools/utxo/wallet_tool/cpp/addr_utils.h"
#include "service/tools/utxo/wallet_tool/cpp/key_utils.h"

namespace py = pybind11;

PYBIND11_MODULE(wallet_tools_py, m) {
  m.doc() = "Nexres Wallet Key Generator";
  m.def("GenECDSAKeys", &resdb::coin::utils::GenECDSAKeys,
        "Generate a ecdsa key pair.");
  m.def(
      "GenAddr",
      [](const std::string& input) {
        std::string output = resdb::coin::utils::GenAddr(input);
        return py::bytes(output.data(), output.size());
      },
      "Generate a addr for a wallet by a public key.");
}
