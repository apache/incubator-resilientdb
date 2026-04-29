#pragma once

#include "platform/consensus/ordering/common/framework/performance_manager.h"

namespace resdb {
namespace hybridset {

class HybridSetPerformanceManager : public common::PerformanceManager {
 public:
  HybridSetPerformanceManager(const ResDBConfig& config,
                               ReplicaCommunicator* replica_communicator,
                               SignatureVerifier* verifier);

 protected:
  void SendMessage(const Request& request);
};

}  // namespace hybridset
}  // namespace resdb
