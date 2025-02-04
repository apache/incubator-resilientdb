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

#include "platform/consensus/ordering/cassandra/algorithm/basic/miner_utils.h"

#include <glog/logging.h>

#include <boost/format.hpp>

#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace cassandra {
namespace basic {

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
  // LOG(ERROR)<<"get number:"<<num<<" diff:"<<difficulty;
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

}  // namespace basic
}  // namespace cassandra
}  // namespace resdb
