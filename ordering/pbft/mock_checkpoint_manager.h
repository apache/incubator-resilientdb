#pragma once

#include <gmock/gmock.h>

#include "ordering/pbft/checkpoint_manager.h"

namespace resdb {

class MockCheckPointManager : public CheckPointManager {
 public:
  MockCheckPointManager(const ResDBConfig& config, ResDBReplicaClient* client,
                        SignatureVerifier* verifier);
  MockCheckPointManager(const std::vector<ReplicaInfo>& replicas)
      : ResDBReplicaClient(replicas) {}

  MOCK_METHOD(int, SendHeartBeat, (const HeartBeatInfo&), (override));
  MOCK_METHOD(int, SendMessage, (const google::protobuf::Message&), (override));

  MOCK_METHOD(void, BroadCast, (const google::protobuf::Message&), (override));
  MOCK_METHOD(void, SendMessage, (const google::protobuf::Message&, int64_t),
              (override));
  MOCK_METHOD(int, SendMessage,
              (const google::protobuf::Message&, const ReplicaInfo&),
              (override));
};

}  // namespace resdb
