#pragma once

#include "platform/consensus/ordering/slot_hs1/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace slot_hs1 {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int slot_num, SignatureVerifier * verifier, int total_num, int fork_tail_num, int rollback_num);

  std::unique_ptr<Proposal> GenerateFakeProposal(const std::vector<std::unique_ptr<Transaction>>& txns, int slot, bool is_final);
  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns, int slot, bool is_final);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView();

  void AddQC(std::unique_ptr<QC> qc);
  std::unique_ptr<Proposal> AddProposal(std::unique_ptr<Proposal> proposal);
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
  int slot_num_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal> > local_block_;

  QC generic_qc_, lock_qc_, fake_generic_qc_;;
  SignatureVerifier* verifier_;

  int total_num_;
  int fork_tail_num_, rollback_num_;

  Stats* global_stats_ = nullptr;
};

}  // namespace slot_hs1
}  // namespace resdb