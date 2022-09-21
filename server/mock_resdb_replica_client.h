#pragma once

#include <gmock/gmock.h>

#include "server/resdb_replica_client.h"

namespace resdb {

class MockResDBReplicaClient : public ResDBReplicaClient {
 public:
  MockResDBReplicaClient() : ResDBReplicaClient({}) {}
  MockResDBReplicaClient(const std::vector<ReplicaInfo>& replicas)
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
