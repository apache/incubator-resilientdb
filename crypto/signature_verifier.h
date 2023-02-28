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

#include <cryptopp/xed25519.h>

#include <shared_mutex>
#include <string>

#include "absl/status/statusor.h"
#include "proto/signature_info.pb.h"

namespace resdb {

// SignatureVerifier used to sign signatures using private_key,
// and verify signatures using the public keys of the senders identified
// by their node_id.
class SignatureVerifier {
 public:
  SignatureVerifier(const KeyInfo& private_key,
                    const CertificateInfo& certificate_info);
  virtual ~SignatureVerifier() = default;

  // Set the public key that contains the public key, node id,
  // and its certificate.
  bool AddPublicKey(const CertificateKey& pub_key, bool need_verify = true);

  // Get the public key of node id. KeyInfo contains the key and its hash type.
  absl::StatusOr<KeyInfo> GetPublicKey(int64_t node_id) const;
  // Get all the public keys.
  std::vector<CertificateKey> GetAllPublicKeys() const;
  // Get the number of public keys it contains.
  size_t GetPublicKeysSize() const;

  // Sign messages using the private key.
  virtual absl::StatusOr<SignatureInfo> SignMessage(const std::string& message);
  absl::StatusOr<SignatureInfo> SignCertificateKeyInfo(
      const CertificateKeyInfo& info);

  // Verify messages using the public key from the sender.
  virtual bool VerifyMessage(const std::string& message,
                             const SignatureInfo& sign);
  bool VerifyMessage(const google::protobuf::Message& message,
                     const SignatureInfo& sign);
  bool VerifyKey(const CertificateKeyInfo& info, const SignatureInfo& sign);

  static std::string CalculateHash(const std::string& str);

  static bool VerifyMessage(const std::string& message,
                            const KeyInfo& public_key,
                            const std::string& signature);

 private:
  std::map<int64_t, CertificateKey> keys_;
  // GUARDED_BY(mutex_);  // public keys of nodes, including the public key and
  // its encrpt type.
  KeyInfo private_key_;       // public-private keys of self.
  KeyInfo admin_public_key_;  // public key of admin.
  int64_t node_id_;           // id of current node.
  std::unique_ptr<CryptoPP::ed25519::Signer> signer_;
  mutable std::shared_mutex mutex_;
};

}  // namespace resdb
