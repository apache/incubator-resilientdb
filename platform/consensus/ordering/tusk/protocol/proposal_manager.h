#pragma once

#include "platform/consensus/ordering/tusk/proto/proposal.pb.h"

namespace resdb {
namespace tusk {

class ProposalManager {
 public:
  ProposalManager(int32_t id, int limit_count, int dag_id, int total_num);

  std::unique_ptr<Proposal> GenerateProposal(
      const std::vector<std::unique_ptr<Transaction>>& txns);

  std::vector<std::unique_ptr<Transaction>> GetTxn();

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

  bool VerifyHash(const Proposal& proposal);

  void Reset(int dag_id);

  int GetLeader(int64_t r);
  bool ReadyWithLeader();

 protected:
  void GetMetaData(Proposal* proposal);

  struct ShardInfo {
   public:
    ShardInfo(int proposer, int round) : proposer(proposer), round(round) {}

    void Add(ShardInfo& info) { shards.push_back(info); }
    std::vector<ShardInfo> shards;
    int shard_flag;
    int proposer;
    int round;
  };

  void AddShardInfo(std::unique_ptr<ShardInfo> shard_info);
  ShardInfo* GetShardInfo(int proposer, int round);
  void MergeShardInfo(ShardInfo* info1, ShardInfo* info2);
  void GenerateCrossGroup(const Proposal& p);

 private:
  int32_t id_;
  std::atomic<int> round_;
  int limit_count_;
  std::map<std::string, std::unique_ptr<Proposal>> block_, local_block_;

  std::map<int64_t, std::map<int, std::unique_ptr<Certificate>>> cert_list_;
  std::map<int, std::unique_ptr<Certificate>> latest_cert_from_sender_;
  std::map<int64_t, int64_t> cert_shard_;

  std::map<std::pair<int, int>, std::unique_ptr<ShardInfo>> shard_info_list_;
  std::mutex txn_mutex_, local_mutex_, round_mutex_, shard_mutex_;
  std::map<std::pair<int, int>, int> reference_;
  const int k_ = 3;
  std::atomic<int> dag_id_;
  const int rotate_k_ = 300;
  int total_num_;
};

}  // namespace tusk
}  // namespace resdb
