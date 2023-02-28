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

#include "crypto/signature_verifier.h"

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
#include <glog/logging.h>

#include "crypto/signature_utils.h"

namespace resdb {

namespace {

// ================== for verify ====================================
bool ED25519verifyString(const std::string& message,
                         const std::string& public_key,
                         const std::string& signature) {
  CryptoPP::byte byteKey[CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH];
  if (public_key.size() != CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH) {
    LOG(ERROR) << "public key len invalid:" << public_key.size();
    return false;
  }
  memcpy(byteKey, public_key.c_str(), public_key.size());
  auto verifier = CryptoPP::ed25519::Verifier(byteKey);

  bool valid = true;
  CryptoPP::StringSource(
      signature + message, true,
      new CryptoPP::SignatureVerificationFilter(
          verifier,
          new CryptoPP::ArraySink((CryptoPP::byte*)&valid, sizeof(valid))));
  if (!valid) {
    LOG(ERROR) << "signature invalid. signature len:" << signature.size()
               << " message len:" << message.size();
  }
  return valid;
}

bool CmacVerifyString(const std::string& message, const std::string& public_key,
                      const std::string& signature) {
  bool res = false;
  // KEY TRANSFORMATION
  // https://stackoverflow.com/questions/26145776/string-to-secbyteblock-conversion
  CryptoPP::SecByteBlock privKey((const unsigned char*)(public_key.data()),
                                 public_key.size());
  CryptoPP::CMAC<CryptoPP::AES> cmac(privKey.data(), privKey.size());
  const int flags = CryptoPP::HashVerificationFilter::HASH_AT_END |
                    CryptoPP::HashVerificationFilter::PUT_RESULT;

  // MESSAGE VERIFICATION
  CryptoPP::StringSource ss3(
      message + signature, true,
      new CryptoPP::HashVerificationFilter(
          cmac, new CryptoPP::ArraySink((CryptoPP::byte*)&res, sizeof(res)),
          flags));  // StringSource
  return res;
}

// ================== for sign ====================================

std::string ED25519signString(const std::string& message,
                              CryptoPP::ed25519::Signer* signer) {
  CryptoPP::AutoSeededRandomPool prng;
  std::string signature;
  CryptoPP::StringSource(
      message, true,
      new CryptoPP::SignerFilter(CryptoPP::NullRNG(), *signer,
                                 new CryptoPP::StringSink(signature)));
  return signature;
}

// CMAC-AES Signature generator
// Return a token, mac, to verify the a message.
std::string CmacSignString(const std::string& private_key,
                           const std::string& message) {
  std::string mac = "";

  // KEY TRANSFORMATION.
  // https://stackoverflow.com/questions/26145776/string-to-secbyteblock-conversion

  CryptoPP::SecByteBlock mac_private_key(
      (const unsigned char*)(private_key.data()), private_key.size());

  CryptoPP::CMAC<CryptoPP::AES> cmac(mac_private_key.data(),
                                     mac_private_key.size());

  CryptoPP::StringSource ss1(
      message, true,
      new CryptoPP::HashFilter(cmac, new CryptoPP::StringSink(mac)));
  return mac;
}

}  // namespace

SignatureVerifier::SignatureVerifier(const KeyInfo& private_key,
                                     const CertificateInfo& certificate_info) {
  private_key_ = private_key;
  if (private_key_.key().empty()) {
    LOG(ERROR) << "private invalid";
  }

  node_id_ = certificate_info.node_id();
  admin_public_key_ = certificate_info.admin_public_key();
  AddPublicKey(certificate_info.public_key(), false);
  if (private_key_.hash_type() == SignatureInfo::ED25519) {
    CryptoPP::byte byteKey[CryptoPP::ed25519PrivateKey::SECRET_KEYLENGTH];
    if (private_key_.key().size() !=
        CryptoPP::ed25519PrivateKey::SECRET_KEYLENGTH) {
      return;
    }
    memcpy(byteKey, private_key_.key().c_str(), private_key_.key().size());
    signer_ = std::make_unique<CryptoPP::ed25519::Signer>(byteKey);
  }
}

bool SignatureVerifier::AddPublicKey(const CertificateKey& public_key,
                                     bool need_verify) {
  std::unique_lock<std::shared_mutex> lk(mutex_);
  if (need_verify &&
      !VerifyKey(public_key.public_key_info(), public_key.certificate())) {
    return false;
  }
  // LOG(ERROR) << "add public key from:"
  //           << public_key.public_key_info().node_id();
  keys_[public_key.public_key_info().node_id()] = public_key;
  return true;
}

absl::StatusOr<KeyInfo> SignatureVerifier::GetPublicKey(int64_t node_id) const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  auto it = keys_.find(node_id);
  if (it == keys_.end()) {
    return absl::InvalidArgumentError("Public key not exist.");
  }
  return it->second.public_key_info().key();
}

std::vector<CertificateKey> SignatureVerifier::GetAllPublicKeys() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  std::vector<CertificateKey> keys;
  for (auto key : keys_) {
    keys.push_back(key.second);
  }
  return keys;
}

size_t SignatureVerifier::GetPublicKeysSize() const {
  std::shared_lock<std::shared_mutex> lk(mutex_);
  return keys_.size();
}

// Funtion to calculate hash of a string.
std::string SignatureVerifier::CalculateHash(const std::string& str) {
  CryptoPP::byte const* pData = (CryptoPP::byte*)str.data();
  unsigned int nDataLen = str.size();
  CryptoPP::byte aDigest[CryptoPP::SHA256::DIGESTSIZE];

  CryptoPP::SHA256().CalculateDigest(aDigest, pData, nDataLen);
  return std::string((char*)aDigest, CryptoPP::SHA256::DIGESTSIZE);
}

absl::StatusOr<SignatureInfo> SignatureVerifier::SignMessage(
    const std::string& message) {
  SignatureInfo info;
  info.set_node_id(node_id_);
  switch (private_key_.hash_type()) {
    case SignatureInfo::RSA:
      info.set_signature(utils::RsaSignString(private_key_.key(), message));
      break;
    case SignatureInfo::ED25519:
      info.set_signature(ED25519signString(message, signer_.get()));
      break;
    case SignatureInfo::CMAC_AES:
      info.set_signature(CmacSignString(private_key_.key(), message));
      break;
    default:
      break;
  }
  return info;
}

bool SignatureVerifier::VerifyMessage(const google::protobuf::Message& message,
                                      const SignatureInfo& sign) {
  std::string str;
  if (!message.SerializeToString(&str)) {
    return false;
  }
  return VerifyMessage(str, sign);
}

bool SignatureVerifier::VerifyMessage(const std::string& message,
                                      const SignatureInfo& info) {
  if (info.signature().empty()) {
    LOG(ERROR) << " signature is empty";
    return false;
  }
  auto public_key = GetPublicKey(info.node_id());
  if (!public_key.ok()) {
    LOG(ERROR) << "key not found:" << info.node_id();
    return false;
  }
  return VerifyMessage(message, *public_key, info.signature());
}

absl::StatusOr<SignatureInfo> SignatureVerifier::SignCertificateKeyInfo(
    const CertificateKeyInfo& info) {
  std::string str;
  if (!info.SerializeToString(&str)) {
    return absl::InvalidArgumentError("Serialize fail");
  }
  return SignatureVerifier::SignMessage(str);
}

bool SignatureVerifier::VerifyKey(const CertificateKeyInfo& info,
                                  const SignatureInfo& sign) {
  std::string str;
  if (!info.SerializeToString(&str)) {
    return false;
  }
  return VerifyMessage(str, admin_public_key_, sign.signature());
}

bool SignatureVerifier::VerifyMessage(const std::string& message,
                                      const KeyInfo& public_key,
                                      const std::string& signature) {
  if (public_key.key().empty() || signature.empty()) {
    LOG(ERROR) << "public is empty, size(" << public_key.key().size()
               << ") or signature is empty, size(" << signature.size() << ")";
    return false;
  }
  // LOG(ERROR) << "public key hash type:" << public_key.hash_type()
  //          << " key size:" << public_key.key().size()
  //         << " sig size:" << signature.size()
  //        << " msg size:" << message.size();
  switch (public_key.hash_type()) {
    case SignatureInfo::RSA:
      return utils::RsaVerifyString(message, public_key.key(), signature);
    case SignatureInfo::ED25519:
      return ED25519verifyString(message, public_key.key(), signature);
    case SignatureInfo::CMAC_AES:
      return CmacVerifyString(message, public_key.key(), signature);
    default:
      return true;
  }
}

}  // namespace resdb
