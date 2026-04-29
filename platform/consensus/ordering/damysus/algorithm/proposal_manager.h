#pragma once

#include "platform/consensus/ordering/damysus/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace damysus {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns,
      const Accumulator& acc,
      const Commitment& block_cert);

  int CurrentView();
  void SetView(int view);
  void AdvanceView();

  void AddBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchBlock(const std::string& hash);

  bool VerifyProposal(const Proposal& proposal);
  bool VerifyHash(const Proposal& proposal);
  std::string ComputeBlockHash(const Proposal& proposal);

 private:
  int32_t id_;
  int view_;
  int limit_count_;  // f+1
  std::map<std::string, std::unique_ptr<Proposal>> blocks_;
  std::mutex mutex_;
  SignatureVerifier* verifier_;
};

}  // namespace damysus
}  // namespace resdb
