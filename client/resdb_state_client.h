#pragma once

#include "absl/status/statusor.h"
#include "client/resdb_client.h"
#include "config/resdb_config.h"
#include "proto/replica_info.pb.h"

namespace resdb {

// ResDBStateClient used to obtain the server state of each replica in ResDB.
// The addresses of each replica are provided from the config.
class ResDBStateClient {
 public:
  ResDBStateClient(const ResDBConfig& config);
  virtual ~ResDBStateClient() = default;

  // Obtain ReplicaState of each replica.
  absl::StatusOr<std::vector<ReplicaState>> GetReplicaStates();

 protected:
  virtual std::unique_ptr<ResDBClient> GetResDBClient(const std::string& ip,
                                                      int port);

 private:
  ResDBConfig config_;
};

}  // namespace resdb
