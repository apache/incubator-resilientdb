#include "crypto/key_generator.h"

#include <cryptopp/aes.h>
#include <cryptopp/cmac.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pssr.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/whrlpool.h>
#include <cryptopp/xed25519.h>
#include <glog/logging.h>

//#include <cryptopp/modes.h>
//#include <iomanip>

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
    default:
      break;
  }
  key.set_hash_type(type);
  return key;
}

}  // namespace resdb
