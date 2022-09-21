#pragma once

#include "ordering/poc/proto/pow.pb.h"

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
