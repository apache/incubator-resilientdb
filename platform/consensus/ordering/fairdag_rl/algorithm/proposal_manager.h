#pragma once

#include "platform/consensus/ordering/fairdag_rl/proto/proposal.pb.h"

namespace resdb {
namespace fairdag_rl {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count);

  std::unique_ptr<Proposal> GenerateProposal(
      std::vector<std::unique_ptr<std::string>>& txns);

  int CurrentRound();
  void AddLocalBlock(std::unique_ptr<Proposal> proposal);
  const Proposal* GetLocalBlock(const std::string& hash);
  std::unique_ptr<Proposal> FetchLocalBlock(const std::string& hash);
  std::unique_ptr<Transaction> FetchTxn(const std::string& hash);
  std::unique_ptr<Transaction> FetchTxnCopy(const std::string& hash);
  void AddTxn(const std::string& hash, std::unique_ptr<Transaction> txn);

  int64_t SetQueuingTime(const std::string& hash);

  void AddBlock(std::unique_ptr<Proposal> proposal);
  void AddCert(std::unique_ptr<Certificate> cert);
  bool Ready();

  const Proposal* GetRequest(int round, int sender);
  std::unique_ptr<Proposal> FetchRequest(int round, int sender);
  int GetReferenceNum(const Proposal& req);

  bool CheckCert(int round, int sender);
  bool CheckBlock(const std::string& hash);

 protected:
  void GetMetaData(Proposal* proposal);

  Transaction* GetTxn(const std::string& hash);

 private:
  int32_t id_;
  int round_;
  int limit_count_;
  std::map<std::string, std::unique_ptr<Proposal>> block_, local_block_;

  std::map<int64_t, std::map<int, std::unique_ptr<Certificate>>> cert_list_;
  std::map<int, std::unique_ptr<Certificate>> latest_cert_from_sender_;

  std::mutex txn_mutex_, local_mutex_;
  std::map<std::pair<int, int>, int> reference_;

  std::mutex data_mutex_;
  std::map<std::string, std::unique_ptr<Transaction>> data_;
  int64_t local_ts_ = 0;
};

}  // namespace fairdag_rl
}  // namespace resdb
