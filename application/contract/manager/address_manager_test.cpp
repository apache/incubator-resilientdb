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

#include "application/contract/manager/address_manager.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "eEVM/util.h"

namespace resdb {
namespace contract {
namespace {

TEST(AddressManagerTest, CreateAddress) {
  Address address = AddressManager().CreateRandomAddress();
  std::array<uint8_t, 32> raw;
  eevm::to_big_endian(address, raw.data());
  Address m = eevm::from_big_endian(raw.data() + 12, raw.size() - 12);
  EXPECT_EQ(m, address);
}

TEST(AddressManagerTest, CreateContractAddress) {
  Address address = AddressManager().CreateRandomAddress();

  Address contract_address = AddressManager::CreateContractAddress(address);

  std::array<uint8_t, 32> raw;
  eevm::to_big_endian(contract_address, raw.data());
  Address m = eevm::from_big_endian(raw.data() + 12, raw.size() - 12);
  EXPECT_EQ(m, contract_address);
}

}  // namespace
}  // namespace contract
}  // namespace resdb
