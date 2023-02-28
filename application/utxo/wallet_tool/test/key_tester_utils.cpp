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
 *
 */

#include <glog/logging.h>
#include <pybind11/pybind11.h>

#include "crypto/signature_utils.h"

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
