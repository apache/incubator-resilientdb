#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/fairdag/algorithm/tusk.h"
#include "platform/consensus/ordering/fairdag/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/fairdag/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace fairdag {

class FairDAG : public common::ProtocolBase {
 public:
  FairDAG(int id, int f, int total_num, SignatureVerifier* verifier);
  ~FairDAG();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);

private:
  void CommitTxns(std::vector<std::unique_ptr<Transaction> > txns);
  void SortTxn(std::vector<std::unique_ptr<Transaction>>& txns, 
      std::map<std::string, uint64_t>& assigned_time);

  uint64_t TryAssign(const std::string& hash);
  uint64_t GetLowerboundTimestamp(const std::string& hash) ;
  uint64_t GetCommitTimestamp(const std::string& hash);

 private:
  std::mutex mutex_;
  std::unique_ptr<Tusk> tusk_;
  int64_t local_time_ = 0;
  std::map<std::string, std::map<int, uint64_t> > committed_txns_;
  std::map<std::string, uint64_t > committed_time_;
  std::set<std::string> ready_;
  int execute_id_;
  std::vector<uint64_t> min_timestamp_;
  int replica_num_;
  std::multimap<uint64_t, std::unique_ptr<Transaction>> not_ready_;
  Stats* global_stats_;
};

}  // namespace tusk
}  // namespace resdb
