#pragma once

#include "crypto/signature_verifier.h"

namespace resdb {

class MockSignatureVerifier : public SignatureVerifier {
 public:
  MockSignatureVerifier() : SignatureVerifier(KeyInfo(), CertificateInfo()) {}
  MOCK_METHOD(absl::StatusOr<SignatureInfo>, SignMessage, (const std::string&),
              (override));
  MOCK_METHOD(bool, VerifyMessage,
              (const std::string&, const SignatureInfo& sign), (override));
};

}  // namespace resdb
