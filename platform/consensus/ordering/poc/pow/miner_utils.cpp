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

#include "platform/consensus/ordering/poc/pow/miner_utils.h"

#include <glog/logging.h>

#include <boost/format.hpp>

#include "common/crypto/signature_verifier.h"
namespace resdb {

HashValue DigestToHash(const std::string& value) {
  HashValue hash;
  const char* pos = value.c_str();
  for (int j = 0; j < 4; ++j) {
    uint64_t v = 0;
    for (int i = 56; i >= 0; i -= 8) {
      uint64_t tmp = *(pos++) & 0xff;
      v |= tmp << i;
    }
    hash.add_bits(v);
  }
  return hash;
}

std::string GetHashDigest(const HashValue& hash) {
  if (hash.bits().empty()) {
    return "";
  }
  char mem[32];
  int mem_idx = 0;
  for (int i = 0; i < 4; ++i) {
    uint64_t bit = hash.bits(i);
    for (int j = 56; j >= 0; j -= 8) {
      mem[mem_idx++] = (bit >> j) & 0xff;
    }
  }
  return std::string(mem, 32);
}

std::string GetDigestHexString(const std::string digest) {
  std::string hex;
  for (size_t i = 0; i < digest.size(); ++i) {
    boost::format fmt("%02x");
    fmt % ((uint32_t)digest[i] & 0xff);
    hex += fmt.str();
  }
  return hex;
}

bool IsValidDigest(const std::string& digest, uint32_t difficulty) {
  uint32_t num = 0;
  for (size_t i = 0; i < digest.size(); ++i) {
    int zeros = 8;
    if (digest[i] == 0) {
    } else {
      uint8_t m = digest[i];
      while (m > 0) {
        zeros--;
        m >>= 1;
      }
    }
    num += zeros;
    if (zeros != 8) break;
  }
  // LOG(ERROR)<<"digest:"<<digest<<" num:"<<num;
  return num >= difficulty;
}

std::string GetHashValue(const std::string& data) {
  return SignatureVerifier::CalculateHash(
      SignatureVerifier::CalculateHash(data));
}

int CmpHash(const HashValue& h1, const HashValue& h2) {
  if (h1.bits_size() != 4 || h2.bits_size() != 4) return -1;
  for (int i = 0; i < 4; ++i) {
    if (h1.bits(i) != h2.bits(i)) {
      return h1.bits(i) < h2.bits(i) ? -1 : 1;
    }
  }
  return 0;
}

bool operator<(const HashValue& h1, const HashValue& h2) {
  return CmpHash(h1, h2) < 0;
}

bool operator<=(const HashValue& h1, const HashValue& h2) {
  return CmpHash(h1, h2) <= 0;
}

bool operator>(const HashValue& h1, const HashValue& h2) {
  return CmpHash(h1, h2) > 0;
}

bool operator>=(const HashValue& h1, const HashValue& h2) {
  return CmpHash(h1, h2) >= 0;
}

bool operator==(const HashValue& h1, const HashValue& h2) {
  return CmpHash(h1, h2) == 0;
}

}  // namespace resdb
