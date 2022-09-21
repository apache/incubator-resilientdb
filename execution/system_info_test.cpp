#include "execution/system_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/test/test_macros.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

ReplicaInfo GenerateReplicaInfo(const std::string& ip, int port,
                                int64_t node_id) {
  ReplicaInfo info;
  info.set_ip(ip);
  info.set_port(port);
  info.set_id(node_id);
  return info;
}

TEST(SystemInfoTest, GetReplicas) {
  std::vector<ReplicaInfo> replicas;
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1234, 1));
  replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1235, 2));

  ResDBConfig config(replicas, GenerateReplicaInfo("127.0.0.1", 1111, 0),
                     KeyInfo(), CertificateInfo());

  SystemInfo system(config);
  EXPECT_THAT(system.GetReplicas(),
              ElementsAre(EqualsProto(replicas[0]), EqualsProto(replicas[1])));

  std::vector<ReplicaInfo> new_replicas;
  new_replicas.push_back(GenerateReplicaInfo("127.0.0.1", 1236, 3));
  system.SetReplicas(new_replicas);

  EXPECT_THAT(system.GetReplicas(), ElementsAre(EqualsProto(new_replicas[0])));

  ReplicaInfo new_replica = GenerateReplicaInfo("127.0.0.1", 1237, 4);
  system.AddReplica(new_replica);

  EXPECT_THAT(system.GetReplicas(), ElementsAre(EqualsProto(new_replicas[0]),
                                                EqualsProto(new_replica)));

  ReplicaInfo deplicate_new_replica = GenerateReplicaInfo("127.0.0.1", 1238, 4);
  system.AddReplica(deplicate_new_replica);
  EXPECT_THAT(system.GetReplicas(), ElementsAre(EqualsProto(new_replicas[0]),
                                                EqualsProto(new_replica)));
}

}  // namespace

}  // namespace resdb
