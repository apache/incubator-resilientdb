#pragma once

#include "config/resdb_config.h"
#include "ordering/pbft/transaction_manager.h"

namespace resdb {

class Query {
 public:
  Query(const ResDBConfig& config, TransactionManager* transaction_manager);
  virtual ~Query();

  virtual int ProcessGetReplicaState(std::unique_ptr<Context> context,
                                     std::unique_ptr<Request> request);
  virtual int ProcessQuery(std::unique_ptr<Context> context,
                           std::unique_ptr<Request> request);

 protected:
  ResDBConfig config_;
  TransactionManager* transaction_manager_;
};

}  // namespace resdb
