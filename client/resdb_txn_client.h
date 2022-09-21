#pragma once

#include "absl/status/statusor.h"
#include "client/resdb_client.h"
#include "config/resdb_config.h"
#include "proto/replica_info.pb.h"

namespace resdb {

// ResDBTxnClient used to obtain the server state of each replica in ResDB.
// The addresses of each replica are provided from the config.
class ResDBTxnClient {
 public:
  ResDBTxnClient(const ResDBConfig& config);
  virtual ~ResDBTxnClient() = default;

  // Obtain ReplicaState of each replica.
  virtual absl::StatusOr<std::vector<std::pair<uint64_t, std::string>>> GetTxn(
      uint64_t min_seq, uint64_t max_seq);

 protected:
  virtual std::unique_ptr<ResDBClient> GetResDBClient(const std::string& ip,
                                                      int port);

 private:
  ResDBConfig config_;
  std::vector<ReplicaInfo> replicas_;
  int recv_timeout_ = 1;
};

}  // namespace resdb
