#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

#include <glog/logging.h>

namespace resdb {
namespace common {

ProtocolBase::ProtocolBase(
    int id, 
    int f,
    int total_num,
    SingleCallFuncType single_call,
    BroadcastCallFuncType broadcast_call,
    CommitFuncType commit) : 
      id_(id), 
      f_(f),
      total_num_(total_num),
      single_call_(single_call), 
      broadcast_call_(broadcast_call), 
      commit_(commit) {
      stop_ = false;
}

ProtocolBase::ProtocolBase( int id, int f, int total_num) : ProtocolBase(id, f, total_num, nullptr, nullptr, nullptr){

}

ProtocolBase::~ProtocolBase() {
  Stop();
}

void ProtocolBase::Stop(){
  stop_ = true;
}

bool ProtocolBase::IsStop(){
  return stop_;
}

bool ProtocolBase::IsSlowReplica(int node_id) {
  return node_id <= network_delay_num_;
}
    
int ProtocolBase::SendMessage(int msg_type, const google::protobuf::Message& msg, int node_id) {
  if (IsSlowReplica(id_)) {
    // 1000, 5000, 50000, 500000
    usleep(GetRandomDelay());
  }
  if (IsSlowReplica(node_id)) {
    usleep(GetRandomDelay());
  }
  return single_call_(msg_type, msg, node_id);
}

int ProtocolBase::Broadcast(int msg_type, const google::protobuf::Message& msg) {
  if (IsSlowReplica(id_)) {
    // 1000, 5000, 50000, 500000
    usleep(GetRandomDelay());
  }
  return broadcast_call_(msg_type, msg);
}

int ProtocolBase::Commit(const google::protobuf::Message& msg) {
  return commit_(msg);
}

void ProtocolBase::SetNetworkDelayGenerator(int32_t network_delay_num, double mean_network_delay) {
  network_delay_num_ = network_delay_num;
  mean_network_delay_ = mean_network_delay;

  sigma_ = 0.2 * mean_network_delay_;  // Standard deviation
  gen_ = std::mt19937(rd_()); // Mersenne Twister engine
  dist_ = std::normal_distribution<>(mean_network_delay_, sigma_);
}

}  // namespace protocol
}  // namespace resdb
