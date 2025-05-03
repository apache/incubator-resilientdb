#pragma once

#include "platform/consensus/ordering/hs_rl/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace hs_rl {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int f, SignatureVerifier * verifier);

  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView();
  bool Ready(int view);

  void AddQC(std::unique_ptr<QC> qc);
  std::unique_ptr<Proposal> AddProposal(std::unique_ptr<Proposal> proposal);
  const Proposal * GetProposal(const std::string& hash);

  std::unique_ptr<Proposal> GenerateLocalOrdering(
      const std::vector<std::unique_ptr<Transaction>>& txns) ;
  bool AddLocalOrdering(std::unique_ptr<Proposal> proposal);
  std::unique_ptr<Proposal> GenerateProposal();

  void AddData(Transaction& txn);
  std::unique_ptr<Transaction> FetchData(const std::string& hash);

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
  int local_round_;
  int f_;
  int limit_count_;
  int proposal_id_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal> > local_block_;
  std::mutex data_mutex_;

  QC generic_qc_, lock_qc_;
  SignatureVerifier* verifier_;
  std::map<int, std::vector<std::unique_ptr<Proposal>> > local_ordering_;
  std::map<std::string, std::unique_ptr<Transaction> > data_;
};

}  // namespace tusk
}  // namespace resdb
