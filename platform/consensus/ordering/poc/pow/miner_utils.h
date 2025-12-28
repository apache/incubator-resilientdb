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

#pragma once

#include "platform/consensus/ordering/poc/proto/pow.pb.h"

namespace resdb {

// Calculate the hash value by sha256(sha256(data))
// and return the binary string.
std::string GetHashValue(const std::string& data);

// Convert the hexadecimal string digest to binary type.
HashValue DigestToHash(const std::string& value);

// Convert the binrary value to the hexadecimal string.
std::string GetHashDigest(const HashValue& hash);

// Convert the hexadecimal string value from the binary value
// obtained from the hash function.
std::string GetDigestHexString(const std::string digest);

// Check if the digest contains 'difficulty' number of zeros.
bool IsValidDigest(const std::string& digest, uint32_t difficulty);

bool operator<(const HashValue& h1, const HashValue& h2);
bool operator<=(const HashValue& h1, const HashValue& h2);
bool operator>(const HashValue& h1, const HashValue& h2);
bool operator>=(const HashValue& h1, const HashValue& h2);
bool operator==(const HashValue& h1, const HashValue& h2);

}  // namespace resdb
