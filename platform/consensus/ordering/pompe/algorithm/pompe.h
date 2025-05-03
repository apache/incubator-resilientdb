#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/statistic/stats.h"
#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/pompe/algorithm/proposal_manager.h"
#include "platform/consensus/ordering/pompe/proto/proposal.pb.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "common/crypto/signature_verifier.h"

namespace resdb {
namespace pompe {

class Pompe : public common::ProtocolBase {
 public:
  Pompe(int id, int f, int total_num, SignatureVerifier* verifier);
  ~Pompe();

  //  recv txn -> send block with links -> rec block ack -> send block with certs
  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  void ReceiveProposal(std::unique_ptr<Proposal> proposal);
  void ReceiveProposalACK(std::unique_ptr<Proposal> proposal);
  void ReceiveProposalTS(std::unique_ptr<Proposal> proposal);
  void ReceiveCollect(std::unique_ptr<Proposal> proposal);

  void ReceivePropose(std::unique_ptr<Proposal> proposal);
  void ReceivePrepare(std::unique_ptr<Proposal> proposal);
  void ReceiveCommit(std::unique_ptr<Proposal> proposal);
  void ReceiveSyncTS(std::unique_ptr<TimeStampSync> sync);

private:
  void AsyncSend();
  void AsyncConsensus();
  void SendProposal();
  void AssignTs(Proposal * proposal);

  void ProcessProposal();
  void SyncTs();
  void TryCollect();


  void DoConsensus();
  void CommitSlot(int slot);
  void AsyncCommit();

 private:
  std::mutex mutex_, sync_mutex_, propose_mutex_, prepare_mutex_, commit_mutex_, slot_mutex_;
  std::mutex cv_mutex_;
  std::map<uint64_t, std::map<int, std::unique_ptr<Proposal>>> received_;
  int replica_num_;
  int batch_size_;
  Stats* global_stats_;
  LockFreeQueue<Transaction> txns_;
  LockFreeQueue<int> commit_queue_;
  std::thread send_thread_, consensu_thread_, sync_thread_;
  std::unique_ptr<ProposalManager> proposal_manager_;
  std::map<int, std::map<int, std::unique_ptr<Proposal>>> pending_proposal_;
  std::map<int, std::set<int>> prepare_received_, commit_received_;
  int primary_;
  int64_t globalSyncTS_; 
  int64_t localSyncTS_;
  int last_slot_;
  int next_slot_;
  int next_commit_slot_;
  std::map<int, std::unique_ptr<Proposal> > propose_data_;
  std::set<int> commit_slot_;
  std::condition_variable vote_cv_;
  int n_;
};

}  // namespace tusk
}  // namespace resdb
