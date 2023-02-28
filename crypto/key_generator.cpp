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

#include "crypto/key_generator.h"

#include <cryptopp/aes.h>
#include <cryptopp/cmac.h>
#include <cryptopp/dsa.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pssr.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/whrlpool.h>
#include <cryptopp/xed25519.h>
#include <glog/logging.h>

namespace resdb {

namespace {

//==============================================================================
// Return a KeyPairHex for each node. This function is used by the nodes to
// obtain their keyPair
SecretKey RsaGenerateHexKeyPair(unsigned int key_size) {
  SecretKey key_pair;
  // PGP Random Pool-like generator
  CryptoPP::AutoSeededRandomPool rng;

  // generate keys
  CryptoPP::RSA::PrivateKey private_key;
  private_key.GenerateRandomWithKeySize(rng, key_size);
  CryptoPP::RSA::PublicKey public_key(private_key);

  // save keys
  public_key.Save(CryptoPP::HexEncoder(
                      new CryptoPP::StringSink(*key_pair.mutable_public_key()))
                      .Ref());
  private_key.Save(CryptoPP::HexEncoder(new CryptoPP::StringSink(
                                            *key_pair.mutable_private_key()))
                       .Ref());

  return key_pair;
}

// Initialize the signer and the keys for ED25519
SecretKey ED25519GenerateKeys() {
  CryptoPP::ed25519::Signer signer;
  SecretKey key_pair;
  CryptoPP::AutoSeededRandomPool prng;
  // Initilize signer;
  signer.AccessPrivateKey().GenerateRandom(prng);

  const CryptoPP::ed25519PrivateKey &private_key =
      dynamic_cast<const CryptoPP::ed25519PrivateKey &>(signer.GetPrivateKey());

  key_pair.mutable_private_key()->assign(
      (char *)private_key.GetPrivateKeyBytePtr(),
      CryptoPP::ed25519PrivateKey::SECRET_KEYLENGTH);
  key_pair.mutable_public_key()->assign(
      (char *)private_key.GetPublicKeyBytePtr(),
      CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH);
  return key_pair;
}

// CMAC-AES key generator
// Return a keyPairHex where just the priv key is set
SecretKey CmacGenerateHexKey(unsigned int key_size) {
  SecretKey key_pair;
  std::string private_key;
  CryptoPP::AutoSeededRandomPool prng;
  // int keySize = 16 ; // AES::DEFAULT_KEYLENGTH
  CryptoPP::SecByteBlock key(key_size);
  prng.GenerateBlock(
      key.data(), key.size());  // Key is a SecByteBlock that contains the key.

  CryptoPP::StringSource ss(key.data(), key.size(), true,
                            new CryptoPP::HexEncoder(new CryptoPP::StringSink(
                                private_key))  // HexEncoder
  );                                           // StringSource
  key_pair.set_private_key(private_key);
  key_pair.set_public_key(private_key);
  return key_pair;
}

SecretKey ECDSAGenerateKeys() {
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;
  CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> params(
      CryptoPP::ASN1::secp256k1());
  privateKey.Initialize(prng, params);

  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
  privateKey.MakePublicKey(publicKey);

  SecretKey key_pair;

  // save keys
  publicKey.Save(CryptoPP::HexEncoder(
                     new CryptoPP::StringSink(*key_pair.mutable_public_key()))
                     .Ref());
  privateKey.Save(CryptoPP::HexEncoder(
                      new CryptoPP::StringSink(*key_pair.mutable_private_key()))
                      .Ref());
  return key_pair;
}

}  // namespace

SecretKey KeyGenerator::GeneratorKeys(SignatureInfo::HashType type) {
  SecretKey key;
  switch (type) {
    case SignatureInfo::RSA:
      key = RsaGenerateHexKeyPair(3072);
      break;
    case SignatureInfo::ED25519: {
      key = ED25519GenerateKeys();
      break;
    }
    case SignatureInfo::CMAC_AES: {
      key = CmacGenerateHexKey(16);
      break;
    }
    case SignatureInfo::ECDSA: {
      key = ECDSAGenerateKeys();
      break;
    }
    default:
      break;
  }
  key.set_hash_type(type);
  return key;
}

}  // namespace resdb
