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

#include "crypto/signature_utils.h"

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
