#pragma once

#include <set>
#include "platform/consensus/ordering/achilles/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace achilles {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns,
      const std::string& parent_hash,
      const Accumulator& acc,
      const BlockCertificate& block_cert);

  std::string ComputeBlockHash(const Proposal& proposal);

  int CurrentView();
  void SetView(int view);
  void AdvanceView();

  void AddBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchBlock(const std::string& hash);

  bool VerifyProposal(const Proposal& proposal);
  bool VerifyHash(const Proposal& proposal);

  // Chained commit support
  void MarkCommitted(const std::string& hash);
  bool IsCommitted(const std::string& hash);
  std::vector<std::string> GetUncommittedAncestors(const std::string& hash);

 private:
  int32_t id_;
  int view_;
  int limit_count_;
  std::map<std::string, std::unique_ptr<Proposal>> blocks_;
  std::set<std::string> committed_;
  std::mutex mutex_;
  SignatureVerifier* verifier_;
};

}  // namespace achilles
}  // namespace resdb
