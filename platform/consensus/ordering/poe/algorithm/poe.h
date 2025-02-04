#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/poe/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace poe {

class PoE : public common::ProtocolBase {
 public:
  PoE(const ResDBConfig& config, int id, int f, int total_num,
      SignatureVerifier* verifier);
  ~PoE();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceivePropose(std::unique_ptr<Transaction> txn);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);
  bool ReceiveSupport(std::unique_ptr<Proposal> proposal);
  bool ReceiveCert(std::unique_ptr<Proposal> proposal);
  bool ReceiveCommit(std::unique_ptr<Proposal> proposal);
  void SetFailFunc(std::function<void(const Transaction& txn)> func);
  void SendFail(const Transaction& txn);
  void AsyncSend();
  void AsyncCommit();

 private:
  bool IsStop();
  bool Verify(const Proposal& proposal);

 private:
  bool linear_ = false;
  std::mutex mutex_[1000], commit_mutex_, smutex_[1000], recv_mutex_[1000],
      commit_recv_mutex_[1000];
  std::map<int64_t, std::set<int32_t>> commit_received_[1000];
  std::map<int64_t, std::set<int32_t>> received_[1000];
  std::map<int64_t, std::vector<std::unique_ptr<Proposal>>>
      support_received_[1000];
  // std::map<std::string, std::set<int32_t> > received_, commit_received_;
  std::map<std::string, std::unique_ptr<Transaction>> data_[1000];
  std::map<std::string, std::unique_ptr<Transaction>> committed_;

  std::mutex cmutex_[1000];
  std::set<int64_t> cert_received_[1000];

  std::thread send_thread_, commit_thread_;

  std::atomic<int64_t> seq_;
  bool is_stop_;
  const ResDBConfig& config_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
  std::atomic<int> commit_seq_;

  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_q_;
  std::function<void(const Transaction& txn)> fail_func_;
};

}  // namespace poe
}  // namespace resdb
