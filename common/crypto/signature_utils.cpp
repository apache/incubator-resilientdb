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

#include "common/crypto/signature_utils.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pssr.h>
#include <cryptopp/rsa.h>
#include <cryptopp/whrlpool.h>
#include <glog/logging.h>

namespace resdb {
namespace utils {

bool RsaVerifyString(const std::string& message, const std::string& public_key,
                     const std::string& signature) {
  try {
    // decode and load public key (using pipeline)
    CryptoPP::RSA::PublicKey rsa_public_key;
    rsa_public_key.Load(
        CryptoPP::StringSource(public_key, true, new CryptoPP::HexDecoder())
            .Ref());

    // decode signature
    std::string decoded_signature;
    CryptoPP::StringSource ss(
        signature, true,
        new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded_signature)));

    // verify message
    bool result = false;
    CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Verifier verifier(
        rsa_public_key);
    CryptoPP::StringSource ss2(
        decoded_signature + message, true,
        new CryptoPP::SignatureVerificationFilter(
            verifier,
            new CryptoPP::ArraySink((CryptoPP::byte*)&result, sizeof(result))));

    return result;
  } catch (...) {
    LOG(ERROR) << "key not valid";
    return false;
  }
}

bool ECDSAVerifyString(const std::string& message,
                       const std::string& public_key,
                       const std::string& signature) {
  std::string decoded;
  std::string output;
  CryptoPP::StringSource(
      signature, true,
      new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded))  // StringSink
  );  // StringSource

  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey dsa_public_key;
  dsa_public_key.Load(
      CryptoPP::StringSource(public_key, true, new CryptoPP::HexDecoder())
          .Ref());
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(
      dsa_public_key);

  bool valid = true;
  CryptoPP::StringSource(
      decoded + message, true,
      new CryptoPP::SignatureVerificationFilter(
          verifier,
          new CryptoPP::ArraySink((CryptoPP::byte*)&valid, sizeof(valid))));
  if (!valid) {
    LOG(ERROR) << "signature invalid. signature len:" << signature.size()
               << " message len:" << message.size();
  }
  return valid;
}

std::string RsaSignString(const std::string& private_key,
                          const std::string& message) {
  // decode and load private key (using pipeline)
  CryptoPP::RSA::PrivateKey rsa_private_key;
  rsa_private_key.Load(
      CryptoPP::StringSource(private_key, true, new CryptoPP::HexDecoder())
          .Ref());

  // sign message
  std::string signature;
  CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Signer signer(
      rsa_private_key);
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::StringSource ss(
      message, true,
      new CryptoPP::SignerFilter(
          rng, signer,
          new CryptoPP::HexEncoder(new CryptoPP::StringSink(signature))));
  return signature;
}

std::string ECDSASignString(const std::string& private_key,
                            const std::string& message) {
  try {
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
    privateKey.Load(
        CryptoPP::StringSource(private_key, true, new CryptoPP::HexDecoder())
            .Ref());
    CryptoPP::AutoSeededRandomPool prng;
    std::string signature;

    CryptoPP::StringSource ss1(
        message, true /*pump all*/,
        new CryptoPP::SignerFilter(
            prng,
            CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer(
                privateKey),
            new CryptoPP::StringSink(signature)));

    std::string output;
    CryptoPP::StringSource(signature, true,
                           new CryptoPP::HexEncoder(
                               new CryptoPP::StringSink(output))  // HexEncoder
    );  // StringSource
    return output;
  } catch (...) {
    LOG(ERROR) << "sign error";
    return "";
  }
}

}  // namespace utils
}  // namespace resdb
