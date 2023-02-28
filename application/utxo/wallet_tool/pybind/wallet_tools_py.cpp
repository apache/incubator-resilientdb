/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <pybind11/pybind11.h>

#include "application/utxo/wallet_tool/cpp/addr_utils.h"
#include "application/utxo/wallet_tool/cpp/key_utils.h"

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
