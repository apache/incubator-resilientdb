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

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"
#include "crypto/key_generator.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;

TEST(SignatureVerifyTest, CalculateHash) {
  std::string expected_str =
      "\x9F\x86\xD0\x81\x88L}e\x9A/"
      "\xEA\xA0\xC5Z\xD0\x15\xA3\xBFO\x1B+\v\x82,\xD1]l\x15\xB0\xF0\n\b";
  EXPECT_EQ(SignatureVerifier::CalculateHash("test"), expected_str);
}

KeyInfo GetKeyInfo(SecretKey key) {
  KeyInfo info;
  info.set_key(key.private_key());
  info.set_hash_type(key.hash_type());
  return info;
}

class SignatureVerifyPTest
    : public ::testing::TestWithParam<SignatureInfo::HashType> {
 public:
  SignatureVerifyPTest() {
    admin_key_ = KeyGenerator ::GeneratorKeys(SignatureInfo::RSA);
    admin_verifier_ = std::make_unique<SignatureVerifier>(
        GetKeyInfo(admin_key_), GetCertInfo(0));
  }

  CertificateInfo GetCertInfo(int64_t node_id) {
    CertificateInfo cert_info;
    cert_info.set_node_id(node_id);

    cert_info.mutable_admin_public_key()->set_key(admin_key_.public_key());
    cert_info.mutable_admin_public_key()->set_hash_type(admin_key_.hash_type());

    return cert_info;
  }

  CertificateKey GetPublicKeyInfo(SecretKey key, int64_t node_id) {
    CertificateKey cert_key;
    cert_key.mutable_public_key_info()->mutable_key()->set_key(
        key.public_key());
    cert_key.mutable_public_key_info()->mutable_key()->set_hash_type(
        key.hash_type());
    cert_key.mutable_public_key_info()->set_node_id(node_id);

    auto resp_info =
        admin_verifier_->SignCertificateKeyInfo(cert_key.public_key_info());
    assert(resp_info.ok());
    *cert_key.mutable_certificate() = *resp_info;
    return cert_key;
  }

 private:
  SecretKey admin_key_;
  std::unique_ptr<SignatureVerifier> admin_verifier_;
};

TEST_P(SignatureVerifyPTest, SignAndVerify) {
  SignatureInfo::HashType type = GetParam();
  absl::StatusOr<SignatureInfo> s_info;
  std::string message = "test_message";
  SecretKey my_key = KeyGenerator ::GeneratorKeys(type);
  int64_t my_node_id = 1, your_node_id = 2;
  {
    SignatureVerifier verifier(GetKeyInfo(my_key), GetCertInfo(my_node_id));
    s_info = verifier.SignMessage(message);
    EXPECT_TRUE(s_info.ok());
    EXPECT_EQ(s_info->node_id(), my_node_id);
  }
  {
    SecretKey your_key = KeyGenerator ::GeneratorKeys(type);
    SignatureVerifier verifier(GetKeyInfo(your_key), GetCertInfo(your_node_id));
    verifier.AddPublicKey(GetPublicKeyInfo(my_key, my_node_id));
    EXPECT_TRUE(verifier.VerifyMessage(message, *s_info));
  }
}

TEST_P(SignatureVerifyPTest, KeyNotFound) {
  SignatureInfo::HashType type = GetParam();

  absl::StatusOr<SignatureInfo> s_info;
  std::string message = "test_message";
  SecretKey my_key = KeyGenerator ::GeneratorKeys(type);
  int64_t my_node_id = 1, your_node_id = 2;

  {
    SignatureVerifier verifier(GetKeyInfo(my_key), GetCertInfo(my_node_id));
    s_info = verifier.SignMessage(message);
    EXPECT_TRUE(s_info.ok());
    EXPECT_EQ(s_info->node_id(), my_node_id);
  }

  {
    SecretKey your_key = KeyGenerator ::GeneratorKeys(type);
    SignatureVerifier verifier(GetKeyInfo(your_key), GetCertInfo(your_node_id));
    EXPECT_FALSE(verifier.VerifyMessage(message, *s_info));
  }
}

TEST_P(SignatureVerifyPTest, KeyNotMatch) {
  SignatureInfo::HashType type = GetParam();

  absl::StatusOr<SignatureInfo> s_info;
  std::string message = "test_message";
  SecretKey my_key = KeyGenerator ::GeneratorKeys(type);
  SecretKey your_key = KeyGenerator ::GeneratorKeys(type);
  int64_t my_node_id = 1, your_node_id = 2;

  {
    SignatureVerifier verifier(GetKeyInfo(my_key), GetCertInfo(my_node_id));
    s_info = verifier.SignMessage(message);
    EXPECT_TRUE(s_info.ok());
    EXPECT_EQ(s_info->node_id(), my_node_id);
  }

  {
    SignatureVerifier verifier(GetKeyInfo(your_key), GetCertInfo(your_node_id));
    verifier.AddPublicKey(GetPublicKeyInfo(your_key, my_node_id));
    EXPECT_FALSE(verifier.VerifyMessage(message, *s_info));
  }
}

INSTANTIATE_TEST_SUITE_P(SignatureVerifyPTest, SignatureVerifyPTest,
                         ::testing::Values(SignatureInfo::RSA,
                                           SignatureInfo::ED25519,
                                           SignatureInfo::CMAC_AES));

}  // namespace

}  // namespace resdb
