#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "common/crypto/signature_verifier.h"
#include "platform/consensus/ordering/spotless/proto/proposal.pb.h"

namespace resdb {
namespace spotless {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int32_t instance_id, int32_t quorum_size,
                  SignatureVerifier* verifier);

  std::unique_ptr<Proposal> GenerateProposal(
      int32_t view, const std::vector<std::unique_ptr<Transaction>>& txns);

  bool VerifyProposal(const Proposal& proposal) const;
  bool VerifyCertificate(const Certificate& cert) const;
  bool UpdateHighestPrepared(const Certificate& cert);

  void AdvanceView(int32_t view);
  int32_t CurrentView() const;
  int32_t InstanceId() const { return instance_id_; }

  void RecordProposal(std::unique_ptr<Proposal> proposal);
  const Proposal* GetProposal(const std::string& hash) const;
  Certificate GetHighestPreparedCert() const;
  Certificate GetLockedCert() const;
  std::unique_ptr<Proposal> MarkPrepared(const Certificate& cert);

  std::string GetHash(const Proposal& proposal) const;

 private:
  struct StoredProposal {
    std::unique_ptr<Proposal> proposal;
    bool prepared = false;
    bool committed = false;
    Certificate prepared_cert;
  };

  bool VerifyHash(const Proposal& proposal) const;
  bool SafeNode(const Proposal& proposal) const;
  void UpdateHighestPreparedLocked(const Certificate& cert);
  void UpdateLockedLocked(const Certificate& cert);

 private:
  int32_t id_;
  int32_t instance_id_;
  int32_t quorum_size_;
  SignatureVerifier* verifier_;

  mutable std::mutex mutex_;
  int32_t current_view_;
  std::map<std::string, StoredProposal> proposals_;
  Certificate highest_prepared_cert_;
  Certificate locked_cert_;
};

}  // namespace spotless
}  // namespace resdb
