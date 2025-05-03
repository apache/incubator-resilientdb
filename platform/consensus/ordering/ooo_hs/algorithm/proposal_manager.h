#pragma once

#include "platform/consensus/ordering/ooo_hs/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace ooohs {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int slot_num, SignatureVerifier * verifier);

  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns, int slot);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView(int slot);

  void AddQC(std::unique_ptr<QC> qc);
  std::vector<std::unique_ptr<Proposal>> AddProposal(std::unique_ptr<Proposal> proposal);
  const Proposal * GetProposal(const std::string& hash);


  protected:
    std::string GetHash(const Proposal& proposal);
    bool SafeNode(const Proposal& proposal);
    bool VerifyQC(const QC& qc);
    bool VerifyHash(const Proposal& proposal);
    std::unique_ptr<Proposal> FetchProposal(int view, int slot);
    void UpdateGenericQC(const QC& qc);

 private:
  int32_t id_;
  std::vector<int> round_;
  int limit_count_;
  int slot_num_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal> > local_block_;
  std::map<std::pair<int,int>, std::string > local_block_hash_;
  std::map<int,int> next_commit_;

  std::vector<QC> generic_qc_, lock_qc_;
  SignatureVerifier* verifier_;
};

}  // namespace ooohs
}  // namespace resdb
