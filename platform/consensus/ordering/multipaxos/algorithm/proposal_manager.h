#pragma once

#include "platform/consensus/ordering/multipaxos/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace multipaxos {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier * verifier);

  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns);
  int CurrentView();

 private:
  int32_t id_;
  int round_;
  int seq_;

  SignatureVerifier* verifier_;
};

}  // namespace tusk
}  // namespace resdb
