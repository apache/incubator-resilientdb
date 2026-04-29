#include "platform/consensus/ordering/hybridset/framework/performance_manager.h"

#include <glog/logging.h>

namespace resdb {
namespace hybridset {

HybridSetPerformanceManager::HybridSetPerformanceManager(
    const ResDBConfig& config, ReplicaCommunicator* replica_communicator,
    SignatureVerifier* verifier)
    : PerformanceManager(config, replica_communicator, verifier) {
}

void HybridSetPerformanceManager::SendMessage(const Request& request) {
  replica_communicator_->BroadCast(request);
}

}  // namespace hybridset
}  // namespace resdb
