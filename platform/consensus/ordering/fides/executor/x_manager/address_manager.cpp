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

#include "service/contract/executor/x_manager/address_manager.h"

#include <glog/logging.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace x_manager {

Address AddressManager::CreateRandomAddress() {
  std::vector<uint8_t> raw(20);
  std::generate(raw.begin(), raw.end(), []() { return rand(); });
  Address address = eevm::from_big_endian(raw.data(), raw.size());
  users_.insert(address);
  return address;
}

bool AddressManager::CreateAddress(const Address& address) {
  if(users_.find(address) != users_.end()){
    return false;
  }
  users_.insert(address);
  return true;
}

bool AddressManager::Exist(const Address& address) {
  return users_.find(address) != users_.end();
}

Address AddressManager::CreateContractAddress(const Address& owner) {
  return eevm::generate_address(owner, 0u);
}

std::string AddressManager::AddressToHex(const Address& address) {
  return eevm::to_hex_string(address);
}

Address AddressManager::HexToAddress(const std::string& address) {
  return eevm::to_uint256(address);
}

uint256_t AddressManager::AddressToSHAKey(const Address& address) {
  std::vector<uint8_t>code;
  code.resize(64u);
  eevm::to_big_endian(address, code.data());
  //code[63]=1;
  //LOG(ERROR)<<"code size:"<<code.size();

  uint8_t h[32];
  eevm::keccak_256(code.data(), static_cast<unsigned int>(code.size()), h);
  //LOG(ERROR)<<"get h:"<<eevm::from_big_endian(h, sizeof(h));
  return eevm::from_big_endian(h, sizeof(h));

}

}
}  // namespace contract
}  // namespace resdb
