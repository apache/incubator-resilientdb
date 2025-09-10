#pragma once

#include "platform/consensus/ordering/hs1/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace hs1 {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, SignatureVerifier * verifier, int total_num, int non_responsive_num, int fork_tail_num, int rollback_num, uint64_t timer_length);

  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns);
  std::unique_ptr<Proposal> GenerateFakeProposal(const std::vector<std::unique_ptr<Transaction>>& txns);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView();

  void AddQC(std::unique_ptr<QC> qc);
  std::vector<std::unique_ptr<Proposal>> AddProposal(std::unique_ptr<Proposal> proposal);
  const Proposal * GetProposal(const std::string& hash);

  int GetLeader(int view);

  protected:
    std::string GetHash(const Proposal& proposal);
    const Proposal * GetHighQC();
    bool SafeNode(const Proposal& proposal);
    bool VerifyQC(const QC& qc);
    bool VerifyHash(const Proposal& proposal);
  std::unique_ptr<Proposal> FetchProposal(const std::string& hash);

 private:
  int32_t id_;
  int round_;
  int limit_count_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal> > local_block_;

  QC generic_qc_, lock_qc_, fake_generic_qc_;
  SignatureVerifier* verifier_;

  int timer_length_;
  int total_num_;
  int non_responsive_num_, fork_tail_num_, rollback_num_;
  Stats* global_stats_ = nullptr;
};

}  // namespace hs
}  // namespace resdb
