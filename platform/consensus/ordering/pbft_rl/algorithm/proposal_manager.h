#pragma once

#include "platform/consensus/ordering/pbft_rl/proto/proposal.pb.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace pbft_rl {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int f, SignatureVerifier * verifier);

  std::unique_ptr<Proposal> GenerateProposal(const std::vector<std::unique_ptr<Transaction>>& txns);
  bool Verify(const Proposal& proposal);
  bool VerifyCert(const Certificate& cert);

  int CurrentView();
  bool Ready(int view);
  void IncView();

  void AddQC(std::unique_ptr<QC> qc);
  std::unique_ptr<Proposal> AddProposal(std::unique_ptr<Proposal> proposal);
  const Proposal * GetProposal(const std::string& hash);

  std::unique_ptr<Proposal> GenerateLocalOrdering(
      const std::vector<std::unique_ptr<Transaction>>& txns) ;
  bool AddLocalOrdering(std::unique_ptr<Proposal> proposal);
  std::unique_ptr<Proposal> GenerateProposal();

  void AddData(Transaction& txn);
  std::unique_ptr<Transaction> FetchTxn(const std::string& hash);

  void ClearUncommittedLocalOrdering(uint64_t round);

  protected:
    std::string GetHash(const Proposal& proposal);
    const Proposal * GetHighQC();
    bool SafeNode(const Proposal& proposal);
    bool VerifyQC(const QC& qc);
    bool VerifyHash(const Proposal& proposal);
  std::unique_ptr<Proposal> FetchProposal(const std::string& hash);

  std::unique_ptr<Transaction> DeepCopyTransaction(const std::unique_ptr<Transaction>& original);
  std::unique_ptr<std::vector<std::unique_ptr<Transaction>>> DeepCopyTransactions(
                  const std::vector<std::unique_ptr<Transaction>>& txns);

  std::set<uint64_t> lowest_position_f_set(const std::map<uint64_t, uint64_t>& m);
  void FilterLocalOrdering(uint64_t view);

 private:
  int32_t id_;
  int round_;
  int local_round_;
  int f_;
  int limit_count_;
  int proposal_id_;

  std::mutex txn_mutex_;
  std::map<std::string, std::unique_ptr<Proposal> > local_block_;
  std::mutex data_mutex_, uncommitted_lo_mutex_;

  QC generic_qc_, lock_qc_;
  SignatureVerifier* verifier_;
  std::map<int, std::vector<std::unique_ptr<Proposal>> > local_ordering_;
  std::map<std::string, std::unique_ptr<Transaction> > data_;

  std::map<uint64_t, std::unique_ptr<std::vector<std::unique_ptr<Transaction>>>> uncommitted_lo_;

  std::map<uint64_t, std::map<uint64_t, uint64_t>> target_position_;
  bool faulty_test_ = true;
  uint64_t faulty_replica_num_ = 1;
  uint64_t batch_size_ = 15;

};

}  // namespace tusk
}  // namespace resdb
