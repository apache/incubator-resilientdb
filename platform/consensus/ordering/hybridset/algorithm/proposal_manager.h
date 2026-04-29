#pragma once

#include "platform/consensus/ordering/hybridset/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace hybridset {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier);

  // Generate a proposal from batched transactions
  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns, int round);

  // Hash computation (excludes TEE certificates)
  std::string ComputeProposalHash(const Proposal& proposal);
  bool VerifyHash(const Proposal& proposal);

  // Verify proposal content — returns VerifResult with valid tx indices and verif_hash
  VerifResult VerifyProposalContent(const Proposal& proposal);

  // Storage keyed by (broadcaster_id, round)
  void StoreProposal(int broadcaster_id, int round,
                     std::unique_ptr<Proposal> proposal);
  const Proposal* GetProposal(int broadcaster_id, int round);
  std::unique_ptr<Proposal> FetchProposal(int broadcaster_id, int round);

 private:
  int32_t id_;
  int limit_count_;
  SignatureVerifier* verifier_;
  std::mutex mutex_;
  std::map<std::pair<int,int>, std::unique_ptr<Proposal>> proposals_;
};

}  // namespace hybridset
}  // namespace resdb
