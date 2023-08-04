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
