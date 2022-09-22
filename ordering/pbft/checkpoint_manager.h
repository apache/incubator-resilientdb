#pragma once

#include "config/resdb_config.h"
#include "crypto/signature_verifier.h"
#include "database/txn_memory_db.h"
#include "proto/checkpoint_info.pb.h"
#include "proto/resdb.pb.h"
#include "server/resdb_replica_client.h"
#include "server/server_comm.h"

namespace resdb {

class CheckPointManager {
 public:
  CheckPointManager(const ResDBConfig& config,
                    ResDBReplicaClient* replica_client,
                    SignatureVerifier* verifier);
  virtual ~CheckPointManager();

  TxnMemoryDB* GetTxnDB();
  uint64_t GetMaxTxnSeq();

  void AddCommitData(std::unique_ptr<Request> request);
  int ProcessCheckPoint(std::unique_ptr<Context> context,
                        std::unique_ptr<Request> request);

  uint64_t GetStableCheckpoint();
  StableCheckPoint GetStableCheckpointWithVotes();
  bool IsValidCheckpointProof(const StableCheckPoint& stable_ckpt);

  void SetTimeoutHandler(std::function<void()> timeout_handler);

 private:
  void UpdateCheckPointStatus();
  void UpdateStableCheckPointStatus();
  void BroadcastCheckPoint(uint64_t seq, const std::string& hash);
  void TimeoutHandler();

  void Notify();
  bool Wait();

 protected:
  ResDBConfig config_;
  ResDBReplicaClient* replica_client_;
  std::unique_ptr<TxnMemoryDB> txn_db_;
  std::thread checkpoint_thread_, stable_checkpoint_thread_;
  SignatureVerifier* verifier_;
  std::atomic<bool> stop_;
  std::map<std::pair<uint64_t, std::string>, std::set<uint32_t>> sender_ckpt_;
  std::map<std::pair<uint64_t, std::string>, std::vector<SignatureInfo>>
      sign_ckpt_;
  std::atomic<uint64_t> current_stable_seq_;
  std::mutex mutex_;
  LockFreeQueue<Request> data_queue_;
  std::mutex cv_mutex_;
  std::condition_variable cv_;
  std::function<void()> timeout_handler_;
  StableCheckPoint stable_ckpt_;
};

}  // namespace resdb
