#pragma once

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/hotstuff_1/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/hotstuff_1/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace hotstuff_1 {

class HotStuff : public common::ProtocolBase {
 public:
  HotStuff(int id, int f, int total_num, int batch_size, SignatureVerifier* verifier);
  ~HotStuff();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveProposal(std::unique_ptr<Proposal> proposal);
  bool ReceiveCertificate(std::unique_ptr<Certificate> cert);
  bool ReceiveStartView(std::unique_ptr<StartView> start_view);

 private:
  bool Ready();
  void StartNewRound();
  void AsyncSend();
  void AsyncCommit();
  void AsyncViewTimeout();
  void SendStartView(int view);
  void UpdateViewTimer(int view);
  std::unique_ptr<Certificate> GenerateCertificate(const Proposal& proposal);
  int NextLeader(int view);
  bool IsLeader(int view);
  void CommitProposal(std::unique_ptr<Proposal> proposal);

 private:
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;

  std::mutex mutex_;
  std::mutex n_mutex_;
  std::condition_variable vote_cv_;
  std::condition_variable timeout_cv_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  bool has_sent_;
  SignatureVerifier* verifier_;

  std::thread send_thread_;
  std::thread commit_thread_;
  std::thread timeout_thread_;

  int batch_size_;
  int timeout_ms_;
  std::map<int, std::map<std::string,
      std::map<int, std::unique_ptr<Certificate>>>> receive_;
  std::map<int, std::set<int>> start_view_ack_;
  std::set<int> received_proposal_views_;
  int tracked_view_;
  int timeout_sent_view_;
  uint64_t view_start_time_;
  Stats* global_stats_;
};

}  // namespace hotstuff_1
}  // namespace resdb
