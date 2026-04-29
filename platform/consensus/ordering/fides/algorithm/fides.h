#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>
#include <fstream>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <iostream>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/fides/proto/proposal.pb.h"
#include "platform/consensus/ordering/fides/algorithm/proposal_manager.h"
#include "enclave/sgx_cpp_u.h"
#include "platform/config/resdb_config.h"
#include "platform/consensus/ordering/fides/algorithm/utility.h"

namespace resdb {
namespace fides {

class Fides : public common::ProtocolBase {
 public:
  Fides(int id, int f, int total_num, SignatureVerifier* verifier,
        const ResDBConfig& config, oe_enclave_t* enclave);
  ~Fides();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBlock(std::unique_ptr<Proposal> proposal);
  void ReceiveBlockACK(std::unique_ptr<Metadata> metadata);
  void ReceiveBlockCert(std::unique_ptr<Certificate> cert);
  bool GetEnclaveStatOnce(uint64_t* current, uint64_t* peak, uint64_t* max);
  void StartEnclaveStatMonitor(const std::string& log_path = "heap_stats.log");
  void StopEnclaveStatMonitor();

 private:
  void CommitProposal(int round, int proposer);
  void CommitRound(int round);
  void AsyncCommit();
  void AsyncSend();
  void AsyncExecute();

  bool VerifyCert(const Certificate& cert);
  bool VerifyCertSimulation(const Certificate& cert);
  bool VerifyCounter(int round, const CounterInfo& counter);

  bool CheckBlock(const Proposal& p);
  void CheckFutureBlock(int round);
  void CheckFutureCert(const Proposal& proposal);
  void CheckFutureCert(int round, const std::string& hash, int proposer);
  bool CheckCert(const Certificate& cert);
  bool SendBlockAck(std::unique_ptr<Proposal> proposal);

  void AsyncProcessCert();

  int GetLeader(int r);

  uint32_t TEECommonCoin(int r);

  // Byzantine behavior helpers
  bool IsByzantine() const;
  void ByzantineBroadcast(int msg_type, const google::protobuf::Message& msg);
  std::vector<int> GetByzantineTargetNodes() const;


 private:
  LockFreeQueue<Proposal> execute_queue_, pending_block_;
  LockFreeQueue<int> commit_queue_;
  LockFreeQueue<Transaction> txns_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  oe_enclave_t* enclave_;

  std::thread send_thread_;
  std::thread commit_thread_, execute_thread_;
  std::mutex txn_mutex_, mutex_;
  int limit_count_;
  std::map<std::string, std::map<int, std::unique_ptr<Metadata>>> received_num_;
  std::condition_variable vote_cv_;
  int start_ = 0;
  int batch_size_ = 0;
  int execute_id_ = 1;
  std::atomic<int> queue_size_;

  Stats* global_stats_;

  std::thread cert_thread_;
  LockFreeQueue<Certificate> cert_queue_;
  std::mutex future_block_mutex_, future_cert_mutex_, check_block_mutex_;
  std::map<int, std::map<std::string, std::unique_ptr<Proposal>>> future_block_;
  std::map<int, std::map<std::string, std::unique_ptr<Certificate>>> future_cert_;
  int64_t  last_commit_time_ = 0;

  std::mutex leader_list_mutex_;
  std::unordered_map<int, int> leader_list_;

  ResDBConfig config_;

  std::optional<std::string> fakemessage_;
  std::optional<resdb::SignatureInfo> fakesign_;

  std::atomic<bool> stop_sampler_{true};
  std::atomic<bool> sampler_running_{false};
  std::thread sampler_thread_;
  std::mutex sampler_mutex_;
};

}  // namespace fides
}  // namespace resdb
