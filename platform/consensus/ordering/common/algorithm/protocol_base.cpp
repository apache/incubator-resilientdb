#include "platform/consensus/ordering/common/algorithm/protocol_base.h"

#include <glog/logging.h>

namespace resdb {
namespace common {

ProtocolBase::ProtocolBase(int id, int f, int total_num,
                           SingleCallFuncType single_call,
                           BroadcastCallFuncType broadcast_call,
                           CommitFuncType commit)
    : id_(id),
      f_(f),
      total_num_(total_num),
      single_call_(single_call),
      broadcast_call_(broadcast_call),
      commit_(commit) {
  stop_ = false;
}

ProtocolBase::ProtocolBase(int id, int f, int total_num)
    : ProtocolBase(id, f, total_num, nullptr, nullptr, nullptr) {}

ProtocolBase::~ProtocolBase() { Stop(); }

void ProtocolBase::Stop() { stop_ = true; }

bool ProtocolBase::IsStop() { return stop_; }

int ProtocolBase::SendMessage(int msg_type,
                              const google::protobuf::Message& msg,
                              int node_id) {
  return single_call_(msg_type, msg, node_id);
}

int ProtocolBase::Broadcast(int msg_type,
                            const google::protobuf::Message& msg) {
  return broadcast_call_(msg_type, msg);
}

int ProtocolBase::Commit(const google::protobuf::Message& msg) {
  return commit_(msg);
}

}  // namespace common
}  // namespace resdb
