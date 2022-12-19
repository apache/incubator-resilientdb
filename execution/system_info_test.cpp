/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

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
