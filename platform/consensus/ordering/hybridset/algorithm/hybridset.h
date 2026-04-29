#pragma once

#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

#include "common/crypto/signature_verifier.h"
#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/hybridset/proto/proposal.pb.h"
#include "platform/consensus/ordering/hybridset/algorithm/proposal_manager.h"
#include "enclave/sgx_cpp_u.h"
#include "platform/config/resdb_config.h"

namespace resdb {
namespace hybridset {

class HybridSet : public common::ProtocolBase {
 public:
  HybridSet(int id, int f, int total_num, SignatureVerifier* verifier,
            const ResDBConfig& config, oe_enclave_t* enclave);
  ~HybridSet();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceiveBcast(std::unique_ptr<BcastMsg> msg);
  bool ReceiveEcho(std::unique_ptr<EchoMsg> msg);
  bool ReceiveVote(std::unique_ptr<VoteMsg> msg);

 private:
  void AsyncSend();
  void AsyncCommit();

  // VRB (Algorithm 1)
  void HandleBcast(int round, const BcastMsg& msg);
  void HandleEcho(int round, const EchoMsg& msg);
  void OnDeliver(int broadcaster_id, int round);

  // Binary BFT (Algorithm 2)
  void HandleVote(int round, const VoteMsg& msg);
  void OnDecide(int broadcaster_id, int round, bool included);
  void CheckAllDecided(int round);
  void VoteZeroForUndelivered(int round);

  // TEE wrappers
  TEECertificate CallTEEbcast(const std::string& proposal_hash);
  TEECertificate CallTEEecho(int broadcaster_id, const std::string& proposal_hash);
  TEECertificate CallTEEvote(int broadcaster_id, const std::string& proposal_hash,
                              bool binary_vote);

  // Per VRB instance state
  struct VRBInstance {
    bool bcast_received = false;
    bool echo_sent = false;
    bool delivered = false;
    std::string proposal_hash;
    std::string verif_hash;
    std::map<int, std::unique_ptr<EchoMsg>> echos;  // sender -> echo
    std::vector<std::unique_ptr<EchoMsg>> buffered_echos;
  };

  // Per Binary BFT instance state
  struct BFTInstance {
    bool voted = false;
    bool decided = false;
    bool included = false;
    std::map<int, std::unique_ptr<VoteMsg>> votes;  // voter -> vote
  };

  // Round state
  struct RoundState {
    std::map<int, VRBInstance> vrb;   // broadcaster_id -> VRBInstance
    std::map<int, BFTInstance> bft;   // broadcaster_id -> BFTInstance
    int deliver_count = 0;
    int decide_count = 0;
    int decide_one_count = 0;
    bool committed = false;
  };

  std::map<int, RoundState> rounds_;
  std::mutex round_mutex_;

  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<Proposal> commit_queue_;

  std::unique_ptr<ProposalManager> proposal_manager_;
  SignatureVerifier* verifier_;
  oe_enclave_t* enclave_;
  ResDBConfig config_;

  std::thread send_thread_;
  std::thread commit_thread_;
  std::condition_variable send_cv_;
  std::mutex send_mutex_;

  int current_round_;
  int limit_count_;   // f+1
  int batch_size_;

  Stats* global_stats_;
  int execute_id_;
};

}  // namespace hybridset
}  // namespace resdb
