#include "ordering/poc/pow/consensus_service_pow.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>

#include "common/test/test_macros.h"
#include "config/resdb_config_utils.h"
#include "crypto/key_generator.h"
#include "crypto/signature_verifier.h"
#include "statistic/stats.h"

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

class MockConsensusServicePoW : public ConsensusServicePoW {
 public:
  MockConsensusServicePoW(const ResDBPoCConfig& config)
      : ConsensusServicePoW(config) {}

  MOCK_METHOD(std::unique_ptr<BatchClientTransactions>, GetClientTransactions,
              (uint64_t), (override));
  MOCK_METHOD(int, BroadCastNewBlock, (const Block&), (override));
  MOCK_METHOD(int, BroadCastShiftMsg, (const SliceInfo&), (override));
  MOCK_METHOD(void, BroadCast, (const Request&), (override));
};

class ConsensusServicePoWTest : public Test {
 protected:
  ConsensusServicePoWTest()
      : stats_(Stats::GetGlobalStats(/*sleep_seconds = */ 1)),
        bft_config_({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                     GenerateReplicaInfo(2, "127.0.0.1", 1235),
                     GenerateReplicaInfo(3, "127.0.0.1", 1236),
                     GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                    GenerateReplicaInfo(3, "127.0.0.1", 1234), KeyInfo(),
                    CertificateInfo()),
        config_(bft_config_,
                {GenerateReplicaInfo(1, "127.0.0.1", 1234),
                 GenerateReplicaInfo(2, "127.0.0.1", 1235),
                 GenerateReplicaInfo(3, "127.0.0.1", 1236),
                 GenerateReplicaInfo(4, "127.0.0.1", 1237)},
                GenerateReplicaInfo(2, "127.0.0.1", 1234), KeyInfo(),
                CertificateInfo()) {
    config_.SetMaxNonceBit(10);
    config_.SetDifficulty(1);
  }

  Stats* stats_;
  ResDBConfig bft_config_;
  ResDBPoCConfig config_;
};

TEST_F(ConsensusServicePoWTest, MineOneBlock) {
  std::mutex mtx;
  std::condition_variable cv;

  MockConsensusServicePoW service(config_);
  EXPECT_CALL(service, GetClientTransactions)
      .WillRepeatedly(Invoke([&](uint64_t seq) {
        std::unique_ptr<BatchClientTransactions> batch_client_request =
            std::make_unique<BatchClientTransactions>();
        ClientTransactions* client_request =
            batch_client_request->add_transactions();
        client_request->set_seq(seq);
        client_request->set_transaction_data("test");

        batch_client_request->set_min_seq(seq);
        batch_client_request->set_max_seq(seq);
        LOG(ERROR) << "get seq:" << seq;
        return batch_client_request;
      }));

  EXPECT_CALL(service, BroadCastNewBlock)
      .WillRepeatedly(Invoke([&](const Block& block) {
        std::unique_lock<std::mutex> lck(mtx);
        cv.notify_all();
        return 0;
      }));

  service.Start();
  {
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
  }
  service.Stop();
}

TEST_F(ConsensusServicePoWTest, ReceiveCommittedBlock) {
  Block new_block;
  // Server 1 mines a new block.
  {
    std::mutex mtx;
    std::condition_variable cv;

    MockConsensusServicePoW service(config_);
    EXPECT_CALL(service, GetClientTransactions)
        .WillRepeatedly(Invoke([&](uint64_t seq) {
          std::unique_ptr<BatchClientTransactions> batch_client_request =
              std::make_unique<BatchClientTransactions>();
          ClientTransactions* client_request =
              batch_client_request->add_transactions();
          client_request->set_seq(seq);
          client_request->set_transaction_data("test");

          batch_client_request->set_min_seq(seq);
          batch_client_request->set_max_seq(seq);
          return batch_client_request;
        }));

    EXPECT_CALL(service, BroadCastNewBlock)
        .WillRepeatedly(Invoke([&](const Block& block) {
          new_block = block;
          std::unique_lock<std::mutex> lck(mtx);
          cv.notify_all();
          return 0;
        }));

    service.Start();
    {
      std::unique_lock<std::mutex> lck(mtx);
      cv.wait(lck);
    }
    service.Stop();
  }

  LOG(ERROR) << "start server 2";
  std::unique_ptr<Request> request = std::make_unique<Request>();
  new_block.SerializeToString(request->mutable_data());
  request->set_type(PoWRequest::TYPE_COMMITTED_BLOCK);

  MockConsensusServicePoW service2(config_);
  service2.Start();
  EXPECT_EQ(
      service2.ConsensusCommit(std::unique_ptr<Context>(), std::move(request)),
      0);
  service2.Stop();
}

/*
TEST_F(ConsensusServicePoWTest, ReceiveShift) {
  std::mutex mtx;
  std::condition_variable cv;

  std::condition_variable shift_cv;

  config_.SetMaxNonceBit(8);
  config_.SetDifficulty(0);

  MockConsensusServicePoW service(config_);
  EXPECT_CALL(service, GetClientTransactions)
      .WillOnce(Invoke([&](uint64_t seq) {
        auto client_request = std::make_unique<ClientTransactions>();
        client_request->set_seq(seq);
        client_request->set_transaction_data("test");
        return client_request;
      }));

  EXPECT_CALL(service, BroadCastShiftMsg)
      .WillRepeatedly(Invoke([&](const SliceInfo& slice_info) {
        std::unique_lock<std::mutex> lck(mtx);
        if(slice_info.shift_idx()==1){
          cv.notify_all();
return 0;
        }
        shift_cv.notify_all();
        return 0;
      }));

  service.Start();
  {
    std::unique_lock<std::mutex> lck(mtx);
    shift_cv.wait(lck);
    for (int i = 0; i < 3; ++i) {
      SliceInfo slice_info;
      slice_info.set_height(1);
      slice_info.set_shift_idx(0);
      slice_info.set_sender(i + 1);

      std::unique_ptr<Request> request = std::make_unique<Request>();
      slice_info.SerializeToString(request->mutable_data());
      request->set_type(PoWRequest::TYPE_SHIFT_MSG);
      service.ConsensusCommit(std::unique_ptr<Context>(), std::move(request));
    }
  }
  {
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
  }
  service.Stop();
}

TEST_F(ConsensusServicePoWTest, ReceiveShiftMore) {
  std::mutex mtx;
  std::condition_variable cv;

  std::condition_variable shift_cv;

  config_.SetMaxNonceBit(7);
  config_.SetDifficulty(0);

  MockConsensusServicePoW service(config_);
  EXPECT_CALL(service, GetClientTransactions)
      .WillOnce(Invoke([&](uint64_t seq) {
        auto client_request = std::make_unique<ClientTransactions>();
        client_request->set_seq(seq);
        client_request->set_transaction_data("test");
        return client_request;
      }));

  EXPECT_CALL(service, BroadCastShiftMsg)
      .WillRepeatedly(Invoke([&](const SliceInfo& slice_info) {
        std::unique_lock<std::mutex> lck(mtx);
        if(slice_info.shift_idx()==2){
          cv.notify_all();
          return 0;
        }
        shift_cv.notify_all();
        return 0;
      }));

  service.Start();
  for(int k = 0; k <2; ++k) {
    std::unique_lock<std::mutex> lck(mtx);
    shift_cv.wait(lck);
      LOG(ERROR)<<"======:"<<k;
    for (int i = 0; i < 3; ++i) {
      SliceInfo slice_info;
      slice_info.set_height(1);
      slice_info.set_shift_idx(k);
      slice_info.set_sender(i + 1);

      std::unique_ptr<Request> request = std::make_unique<Request>();
      slice_info.SerializeToString(request->mutable_data());
      request->set_type(PoWRequest::TYPE_SHIFT_MSG);
      service.ConsensusCommit(std::unique_ptr<Context>(), std::move(request));
    }
  }
  {
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
  }
  service.Stop();
}

// The shift messages arrive before the mining id done.
TEST_F(ConsensusServicePoWTest, ReceiveShiftEarly) {
  std::mutex mtx;
  std::condition_variable cv;

  std::condition_variable shift_cv;

  config_.SetMaxNonceBit(8);
  config_.SetDifficulty(0);

  MockConsensusServicePoW service(config_);
  EXPECT_CALL(service, GetClientTransactions)
      .WillRepeatedly(Invoke([&](uint64_t seq) {
        auto client_request = std::make_unique<ClientTransactions>();
        client_request->set_seq(seq);
        client_request->set_transaction_data("test");
        return client_request;
      }));

  EXPECT_CALL(service, BroadCastShiftMsg)
      .WillOnce(Invoke([&](const SliceInfo& slice_info) {
        // Receive the shift messages first.
        {
          for (int i = 0; i < 3; ++i) {
            SliceInfo slice_info;
            slice_info.set_height(1);
            slice_info.set_shift_idx(1);
            slice_info.set_sender(i + 1);

            std::unique_ptr<Request> request = std::make_unique<Request>();
            slice_info.SerializeToString(request->mutable_data());
            request->set_type(PoWRequest::TYPE_SHIFT_MSG);
            service.ConsensusCommit(std::unique_ptr<Context>(),
                                    std::move(request));
          }
        }
        return 0;
      }));

  EXPECT_CALL(service, BroadCastNewBlock)
      .WillRepeatedly(Invoke([&](const Block& block) {
        std::unique_lock<std::mutex> lck(mtx);
        cv.notify_all();
        return 0;
      }));
  service.Start();
  {
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
  }
  service.Stop();
}
*/

}  // namespace
}  // namespace resdb
