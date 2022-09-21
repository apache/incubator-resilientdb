#pragma once

#include "config/resdb_config.h"
#include "crypto/signature_verifier.h"
#include "ordering/pbft/transaction_manager.h"
#include "server/resdb_replica_client.h"

namespace resdb {

class Recovery {
 public:
  Recovery(const ResDBConfig& config, TransactionManager* transaction_manager,
           ResDBReplicaClient* replica_client, SignatureVerifier* verifier);
  virtual ~Recovery();

  virtual int ProcessRecoveryDataResp(std::unique_ptr<Context> context,
                                      std::unique_ptr<Request> request);

  virtual int ProcessRecoveryData(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request);

  virtual void AddNewReplica(const ReplicaInfo& info);

 private:
  void HealthCheck();
  bool IsPrimary();

 protected:
  ResDBConfig config_;
  TransactionManager* transaction_manager_;
  std::thread healthy_thread_;
  ResDBReplicaClient* replica_client_;
  SignatureVerifier* verifier_;
  std::atomic<bool> stop_;
};

}  // namespace resdb
