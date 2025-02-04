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

#include "platform/consensus/ordering/poc/pow/block_manager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/crypto/key_generator.h"
#include "common/crypto/signature_verifier.h"
#include "common/test/test_macros.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/poc/pow/merkle.h"

namespace resdb {
namespace {

using ::resdb::testing::EqualsProto;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::Pair;
using ::testing::Pointee;
using ::testing::Test;

KeyInfo GetKeyInfo(SecretKey key) {
  KeyInfo info;
  info.set_key(key.private_key());
  info.set_hash_type(key.hash_type());
  return info;
}

KeyInfo GetPublicKeyInfo(SecretKey key) {
  KeyInfo info;
  info.set_key(key.public_key());
  info.set_hash_type(key.hash_type());
  return info;
}

CertificateInfo GetCertInfo(int64_t node_id) {
  CertificateInfo cert_info;
  cert_info.set_node_id(node_id);
  return cert_info;
}

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

class BlockManagerTest : public Test {
 protected:
  BlockManagerTest()
      : bft_config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                     GenerateReplicaInfo(2, "127.0.0.1", 1235),
                     GenerateReplicaInfo(3, "127.0.0.1", 1236),
                     GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                    GenerateReplicaInfo(3, "127.0.0.1", 1234), KeyInfo(),
                    CertificateInfo()),
        config_(bft_config_,
                GetConfigData({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                               GenerateReplicaInfo(2, "127.0.0.1", 1235),
                               GenerateReplicaInfo(3, "127.0.0.1", 1236),
                               GenerateReplicaInfo(4, "127.0.0.1", 1237)}),
                GenerateReplicaInfo(1, "127.0.0.1", 1234), KeyInfo(),
                CertificateInfo()) {
    config_.SetMaxNonceBit(10);
    config_.SetDifficulty(1);
  }

  Block* GenerateNewBlock(
      BlockManager* block_manager,
      std::unique_ptr<BatchClientTransactions> client_request) {
    if (block_manager->SetNewMiningBlock(std::move(client_request)) != 0) {
      return nullptr;
    }
    return block_manager->GetNewMiningBlock();
  }

  Block GenerateExpectedBlock(
      const BatchClientTransactions& batch_client_request, int height) {
    Block expected_block;
    expected_block.mutable_header()->set_height(height);
    batch_client_request.SerializeToString(
        expected_block.mutable_transaction_data());
    *expected_block.mutable_header()->mutable_pre_hash() = HashValue();
    *expected_block.mutable_header()->mutable_merkle_hash() =
        Merkle::MakeHash(batch_client_request);
    expected_block.set_min_seq(batch_client_request.min_seq());
    expected_block.set_max_seq(batch_client_request.max_seq());
    expected_block.set_miner(1);
    return expected_block;
  }

  ResDBConfig bft_config_;
  ResDBPoCConfig config_;
};

TEST_F(BlockManagerTest, GenerateNewBlock) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
}

TEST_F(BlockManagerTest, ResetSliceFail) {
  BlockManager block_manager(config_);
  BatchClientTransactions batch_client_request;
  ClientTransactions* client_request = batch_client_request.add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request.set_min_seq(1);
  batch_client_request.set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(batch_client_request, 1);

  Block* block = GenerateNewBlock(
      &block_manager,
      std::make_unique<BatchClientTransactions>(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

  SliceInfo slice_info;
  slice_info.set_shift_idx(2);
  slice_info.set_height(2);
  block_manager.SetSliceIdx(slice_info);
  EXPECT_EQ(block_manager.GetSliceIdx(), 0);
}

TEST_F(BlockManagerTest, ResetSlice) {
  BlockManager block_manager(config_);
  BatchClientTransactions batch_client_request;
  ClientTransactions* client_request = batch_client_request.add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request.set_min_seq(1);
  batch_client_request.set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(batch_client_request, 1);

  Block* block = GenerateNewBlock(
      &block_manager,
      std::make_unique<BatchClientTransactions>(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

  SliceInfo slice_info;
  slice_info.set_shift_idx(2);
  slice_info.set_height(1);
  block_manager.SetSliceIdx(slice_info);
  EXPECT_EQ(block_manager.GetSliceIdx(), 0);
}

// Merge the current one to next due to f+1 rounds
TEST_F(BlockManagerTest, MergeTxn) {
  BlockManager block_manager(config_);
  BatchClientTransactions batch_client_request;
  ClientTransactions* client_request = batch_client_request.add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request.set_min_seq(1);
  batch_client_request.set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(batch_client_request, 1);

  Block* block = GenerateNewBlock(
      &block_manager,
      std::make_unique<BatchClientTransactions>(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

  SliceInfo slice_info;
  slice_info.set_shift_idx(2);
  slice_info.set_height(1);
  EXPECT_EQ(block_manager.SetSliceIdx(slice_info), 1);
  EXPECT_EQ(block_manager.GetSliceIdx(), 0);

  {
    BatchClientTransactions new_batch_client_request;
    ClientTransactions* client_request1 =
        new_batch_client_request.add_transactions();
    client_request1->set_seq(1);
    client_request1->set_transaction_data("test");

    ClientTransactions* client_request2 =
        new_batch_client_request.add_transactions();
    client_request2->set_seq(2);
    client_request2->set_transaction_data("test");

    new_batch_client_request.set_min_seq(1);
    new_batch_client_request.set_max_seq(2);
    expected_block = GenerateExpectedBlock(new_batch_client_request, 1);
  }

  // Get the merged txn with seq [1,2]
  {
    BatchClientTransactions batch_client_request2;
    ClientTransactions* client_request =
        batch_client_request2.add_transactions();
    client_request->set_seq(2);
    client_request->set_transaction_data("test");
    batch_client_request2.set_min_seq(2);
    batch_client_request2.set_max_seq(2);

    block = GenerateNewBlock(
        &block_manager,
        std::make_unique<BatchClientTransactions>(batch_client_request2));
    block->clear_block_time();
    EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  }
}

// successfully find out the solution.
TEST_F(BlockManagerTest, Mine) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

  EXPECT_TRUE(block_manager.Mine().ok());
}

// could not find the solution.
TEST_F(BlockManagerTest, MineFail) {
  config_.SetDifficulty(20);
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

  EXPECT_FALSE(block_manager.Mine().ok());
}

// seq is too larger.
TEST_F(BlockManagerTest, MineSeqNotValid) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(2);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(2);
  batch_client_request->set_max_seq(2);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  EXPECT_EQ(block, nullptr);
}

// Commit a mined block.
TEST_F(BlockManagerTest, Commit) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);

  EXPECT_EQ(block_manager.Commit(), 0);

  EXPECT_TRUE(block_manager.GetBlockByHeight(1) != nullptr);
}

TEST_F(BlockManagerTest, MineACommittedSeq) {
  BlockManager block_manager(config_);
  {
    // Commit seq 1
    std::unique_ptr<BatchClientTransactions> batch_client_request =
        std::make_unique<BatchClientTransactions>();
    auto client_request = batch_client_request->add_transactions();
    client_request->set_seq(1);
    client_request->set_transaction_data("test");
    batch_client_request->set_min_seq(1);
    batch_client_request->set_max_seq(1);

    Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

    Block* block =
        GenerateNewBlock(&block_manager, std::move(batch_client_request));
    block->clear_block_time();
    EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));

    EXPECT_TRUE(block_manager.Mine().ok());
    EXPECT_EQ(block_manager.Commit(), 0);
  }
  {
    // Fail to mind seq 1 again.
    std::unique_ptr<BatchClientTransactions> batch_client_request =
        std::make_unique<BatchClientTransactions>();
    auto client_request = batch_client_request->add_transactions();
    client_request->set_seq(1);
    client_request->set_transaction_data("test");
    batch_client_request->set_min_seq(1);
    batch_client_request->set_max_seq(1);

    Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

    Block* block =
        GenerateNewBlock(&block_manager, std::move(batch_client_request));
    EXPECT_EQ(block, nullptr);
  }
}

// Commit a mined block with invalid height.
TEST_F(BlockManagerTest, CommitWithInvalidHeight) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);

  std::unique_ptr<Block> old_block = std::make_unique<Block>(*block);
  old_block->mutable_header()->set_height(0);
  EXPECT_EQ(block_manager.Commit(), 0);
  EXPECT_NE(block_manager.Commit(std::move(old_block)), 0);
}

// Mine a block after it is committed from others.
TEST_F(BlockManagerTest, MineCommittedBlock) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  std::unique_ptr<Block> d_block = std::make_unique<Block>(*block);

  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);

  EXPECT_EQ(block_manager.Commit(std::move(d_block)), 0);

  // mind the committed block, d_block
  EXPECT_FALSE(block_manager.Mine().ok());
}

// Verify a mined block with invalid hash.
TEST_F(BlockManagerTest, VerifyWithInvalidHash) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);

  *block->mutable_hash() = HashValue();
  EXPECT_FALSE(block_manager.VerifyBlock(block));
}

TEST_F(BlockManagerTest, VerifyBlockWithInvalidTxn) {
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);

  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  block->clear_block_time();
  EXPECT_THAT(block, Pointee(EqualsProto(expected_block)));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);
  block->set_transaction_data("test1");

  EXPECT_FALSE(block_manager.VerifyBlock(block));
}

TEST_F(BlockManagerTest, VerifyBlock) {
  config_.SetWorkerNum(1);
  BlockManager block_manager(config_);
  std::unique_ptr<BatchClientTransactions> batch_client_request =
      std::make_unique<BatchClientTransactions>();
  auto client_request = batch_client_request->add_transactions();
  client_request->set_seq(1);
  client_request->set_transaction_data("test");
  batch_client_request->set_min_seq(1);
  batch_client_request->set_max_seq(1);

  Block expected_block = GenerateExpectedBlock(*batch_client_request, 1);
  Block* block =
      GenerateNewBlock(&block_manager, std::move(batch_client_request));
  EXPECT_TRUE(block_manager.Mine().ok());
  EXPECT_GT(block->header().nonce(), 0);

  EXPECT_TRUE(block_manager.VerifyBlock(block));
}

}  // namespace
}  // namespace resdb
