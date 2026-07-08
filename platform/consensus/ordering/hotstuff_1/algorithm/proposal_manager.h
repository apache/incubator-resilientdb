#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/crypto/signature_verifier.h"
#include "platform/consensus/ordering/hotstuff_1/proto/proposal.pb.h"

namespace resdb {
namespace hotstuff_1 {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier* verifier);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView();
  void AdvanceView(int view);

  void AddQC(std::unique_ptr<QC> qc);
  std::unique_ptr<Proposal> AddProposal(std::unique_ptr<Proposal> proposal);
  const Proposal* GetProposal(const std::string& hash);

 protected:
  std::string GetHash(const Proposal& proposal);
  const Proposal* GetHighQC();
  bool SafeNode(const Proposal& proposal);
  bool VerifyQC(const QC& qc);
  bool VerifyHash(const Proposal& proposal);
  std::unique_ptr<Proposal> FetchProposal(const std::string& hash);

 private:
  int32_t id_;
  int round_;
  int limit_count_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal>> local_block_;

  QC generic_qc_;
  QC lock_qc_;
  SignatureVerifier* verifier_;
};

}  // namespace hotstuff_1
}  // namespace resdb
