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

#include "platform/consensus/ordering/poc/pow/miner.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/format.hpp>

#include "common/test/test_macros.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/poc/pow/miner_utils.h"

namespace resdb {
namespace {

using ::google::protobuf::util::MessageDifferencer;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::Test;

ResConfigData GetConfigData(const std::vector<ReplicaInfo>& replicas) {
  ResConfigData config_data;
  auto region = config_data.add_region();
  region->set_region_id(1);
  for (auto replica : replicas) {
    *region->add_replica_info() = replica;
  }
  config_data.set_self_region_id(1);
  return config_data;
}

class MinerTest : public Test {
 public:
  ResDBPoCConfig GetConfig(int idx) {
    ResDBConfig bft_config({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                            GenerateReplicaInfo(2, "127.0.0.1", 1235),
                            GenerateReplicaInfo(3, "127.0.0.1", 1236),
                            GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                           GenerateReplicaInfo(idx, "127.0.0.1", 1234),
                           KeyInfo(), CertificateInfo());

    return ResDBPoCConfig(
        bft_config,
        GetConfigData({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                       GenerateReplicaInfo(2, "127.0.0.1", 1235),
                       GenerateReplicaInfo(3, "127.0.0.1", 1236),
                       GenerateReplicaInfo(4, "127.0.0.1", 1237)}),
        GenerateReplicaInfo(idx, "127.0.0.1", 1234), KeyInfo(),
        CertificateInfo());
  }

  ResDBPoCConfig GetConfig(int size, int idx) {
    std::vector<ReplicaInfo> replicas;
    for (int i = 0; i < size; ++i) {
      replicas.push_back(GenerateReplicaInfo(i + 1, "127.0.0.1", 1234 + i));
    }
    ResDBConfig bft_config(replicas,
                           GenerateReplicaInfo(idx, "127.0.0.1", 1234),
                           KeyInfo(), CertificateInfo());

    return ResDBPoCConfig(bft_config, GetConfigData(replicas),
                          GenerateReplicaInfo(idx, "127.0.0.1", 1234),
                          KeyInfo(), CertificateInfo());
  }
};

TEST_F(MinerTest, Slices) {
  for (int i = 1; i <= 4; ++i) {
    ResDBPoCConfig config = GetConfig(i);
    config.SetMaxNonceBit(3);
    config.SetDifficulty(1);
    Miner miner(config);

    std::vector<std::pair<uint64_t, uint64_t>> slices = miner.GetMiningSlices();
    EXPECT_THAT(slices, ElementsAre(Pair((i - 1) * 2, (i - 1) * 2 + 1)));
  }
}

TEST_F(MinerTest, AddNewBlock) {
  ResDBPoCConfig config = GetConfig(1, 1);
  config.SetMaxNonceBit(30);
  config.SetDifficulty(7);
  Miner miner(config);

  Block block;
  block.mutable_header()->set_height(1);

  EXPECT_TRUE(miner.Mine(&block).ok());
}

TEST_F(MinerTest, Mine) {
  ResDBPoCConfig config = GetConfig(3);
  config.SetMaxNonceBit(20);
  config.SetDifficulty(2);
  config.SetWorkerNum(1);
  Miner miner(config);

  Block block;
  *block.mutable_header()->mutable_pre_hash() =
      DigestToHash(GetHashValue("pre_hash"));
  *block.mutable_header()->mutable_merkle_hash() =
      DigestToHash(GetHashValue("merkle_hash"));
  block.mutable_header()->set_height(1);
  block.mutable_header()->set_nonce(524292);

  std::string expected_hash_header;
  expected_hash_header += GetHashDigest(block.header().pre_hash());
  expected_hash_header += GetHashDigest(block.header().merkle_hash());
  expected_hash_header += std::to_string(block.header().nonce());

  std::string expected_hash = GetHashValue(expected_hash_header);

  EXPECT_TRUE(miner.Mine(&block).ok());
  EXPECT_TRUE(
      MessageDifferencer::Equals(block.hash(), DigestToHash(expected_hash)));
}

TEST_F(MinerTest, MineFail) {
  ResDBPoCConfig config = GetConfig(3);
  config.SetMaxNonceBit(3);
  config.SetDifficulty(2);
  Miner miner(config);

  Block block;
  block.mutable_header()->set_height(1);

  absl::Status status = miner.Mine(&block);
  EXPECT_FALSE(status.ok());
}

TEST_F(MinerTest, HashValid) {
  ResDBPoCConfig config = GetConfig(3);
  config.SetMaxNonceBit(3);
  config.SetDifficulty(2);
  Miner miner(config);

  Block block;
  *block.mutable_header()->mutable_pre_hash() =
      DigestToHash(GetHashValue("pre_hash"));
  *block.mutable_header()->mutable_merkle_hash() =
      DigestToHash(GetHashValue("merkle_hash"));
  block.mutable_header()->set_height(1);
  block.mutable_header()->set_nonce(4);

  std::string expected_hash_header;
  expected_hash_header += GetHashDigest(block.header().pre_hash());
  expected_hash_header += GetHashDigest(block.header().merkle_hash());
  expected_hash_header += std::to_string(block.header().nonce());

  std::string expected_hash = GetHashValue(expected_hash_header);
  *block.mutable_hash() = DigestToHash(expected_hash);
  EXPECT_TRUE(miner.IsValidHash(&block));
}

TEST_F(MinerTest, HashNotValid) {
  ResDBPoCConfig config = GetConfig(3);
  config.SetMaxNonceBit(3);
  config.SetDifficulty(2);
  Miner miner(config);

  Block block;
  *block.mutable_header()->mutable_pre_hash() =
      DigestToHash(GetHashValue("pre_hash"));
  *block.mutable_header()->mutable_merkle_hash() =
      DigestToHash(GetHashValue("merkle_hash"));
  block.mutable_header()->set_height(1);
  block.mutable_header()->set_nonce(4);

  std::string expected_hash_header;
  expected_hash_header += GetHashDigest(block.header().pre_hash());
  expected_hash_header += GetHashDigest(block.header().merkle_hash());
  expected_hash_header += std::to_string(block.header().nonce());

  std::string expected_hash = GetHashValue(expected_hash_header);
  *block.mutable_hash() = DigestToHash(expected_hash);
  block.mutable_header()->set_nonce(5);

  EXPECT_FALSE(miner.IsValidHash(&block));
}

}  // namespace
}  // namespace resdb
