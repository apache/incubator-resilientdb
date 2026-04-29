#pragma once

#include "platform/consensus/ordering/bullshark/proto/proposal.pb.h"

namespace resdb {
namespace bullshark {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int total_num);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);

  int CurrentRound();
  void AddLocalBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetLocalBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchLocalBlock(const std::string& hash);

  void AddBlock(std::unique_ptr<Proposal> proposal);
  void AddCert(std::unique_ptr<Certificate> cert);
  bool Ready();

  const Proposal* GetRequest(int round, int sender);
  std::unique_ptr<Proposal> FetchRequest(int round, int sender);
  int GetReferenceNum(const Proposal& req);

  bool VerifyHash(const Proposal &proposal);

  bool CheckCert(int round, int sender);
  bool CheckBlock(const std::string& hash);

  void SetCommittedRound(int r);

  // BullShark: check if there is a strong path (only strong edges)
  // from (from_round, from_proposer) to (to_round, to_proposer) in the DAG.
  bool HasStrongPath(int from_round, int from_proposer,
                     int to_round, int to_proposer);

 protected:
  void GetMetaData(Proposal* proposal);

 private:
  int32_t id_;
  int round_;
  int limit_count_;
  int total_num_;
  std::map<std::string, std::unique_ptr<Proposal>> block_, local_block_;

  std::map<int64_t, std::map<int, std::unique_ptr<Certificate>>> cert_list_;
  std::map<int, std::unique_ptr<Certificate>> latest_cert_from_sender_;

  std::mutex txn_mutex_, local_mutex_;
  std::map<std::pair<int, int>, int> reference_;

  std::atomic<int> committed_round_ = 0;
};

}  // namespace bullshark
}  // namespace resdb
