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

#include "crypto/hash.h"

#include <cryptopp/ripemd.h>
#include <cryptopp/sha.h>
#include <glog/logging.h>

namespace resdb {
namespace utils {
// Funtion to calculate hash of a string.
std::string CalculateSHA256Hash(const std::string& str) {
  CryptoPP::byte const* pData = (CryptoPP::byte*)str.data();
  unsigned int nDataLen = str.size();
  CryptoPP::byte aDigest[CryptoPP::SHA256::DIGESTSIZE];

  CryptoPP::SHA256().CalculateDigest(aDigest, pData, nDataLen);
  return std::string((char*)aDigest, CryptoPP::SHA256::DIGESTSIZE);
}

std::string CalculateRIPEMD160Hash(const std::string& str) {
  CryptoPP::byte const* pData = (CryptoPP::byte*)str.data();
  unsigned int nDataLen = str.size();
  CryptoPP::byte aDigest[CryptoPP::RIPEMD160::DIGESTSIZE];

  CryptoPP::RIPEMD160().CalculateDigest(aDigest, pData, nDataLen);
  return std::string((char*)aDigest, CryptoPP::RIPEMD160::DIGESTSIZE);
}

}  // namespace utils
}  // namespace resdb
