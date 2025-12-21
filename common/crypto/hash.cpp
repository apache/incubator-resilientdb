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

#include "common/crypto/hash.h"

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
