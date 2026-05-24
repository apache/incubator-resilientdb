#include "platform/consensus/ordering/sharded_3pc/shard_poe_commitment.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/crypto/signature_verifier.h"
#include "executor/common/transaction_manager.h"
#include "platform/config/resdb_config_utils.h"
#include "platform/consensus/ordering/pbft/checkpoint_manager.h"
#include "platform/consensus/ordering/pbft/message_manager.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
namespace {

ResDBConfig GenerateConfig(int64_t self_id) {
  ResConfigData data;
  data.set_duplicate_check_frequency_useconds(100000);
  return ResDBConfig({GenerateReplicaInfo(1, "127.0.0.1", 1234),
                      GenerateReplicaInfo(2, "127.0.0.1", 1235),
                      GenerateReplicaInfo(3, "127.0.0.1", 1236),
                      GenerateReplicaInfo(4, "127.0.0.1", 1237),
                      GenerateReplicaInfo(5, "127.0.0.1", 1238),
                      GenerateReplicaInfo(6, "127.0.0.1", 1239),
                      GenerateReplicaInfo(7, "127.0.0.1", 1240),
                      GenerateReplicaInfo(8, "127.0.0.1", 1241)},
                     GenerateReplicaInfo(self_id, "127.0.0.1",
                                         1233 + self_id),
                     data);
}

std::string WriteShardConfig() {
  static int file_count = 0;
  const char* test_tmpdir = std::getenv("TEST_TMPDIR");
  const std::string directory = test_tmpdir == nullptr ? "/tmp" : test_tmpdir;
  const std::string path = directory + "/shard_poe_commitment_test_" +
                           std::to_string(getpid()) + "_" +
                           std::to_string(file_count++) + ".json";
  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::runtime_error("failed to write test shard config " + path);
  }
  output << R"json(
{
  "shards": [
    {"shard_id": 0, "leader_id": 1, "replica_ids": [1, 2, 3, 4]},
    {"shard_id": 1, "leader_id": 5, "replica_ids": [5, 6, 7, 8]}
  ],
  "client_ids": [9]
}
)json";
  return path;
}

class RecordingReplicaCommunicator : public ReplicaCommunicator {
 public:
  RecordingReplicaCommunicator() : ReplicaCommunicator({}) {}

  int SendMessage(const google::protobuf::Message& message,
                  const ReplicaInfo& replica) override {
    std::lock_guard<std::mutex> lk(mutex_);
    sent_node_ids.push_back(replica.id());
    const Request* request = dynamic_cast<const Request*>(&message);
    if (request != nullptr) {
      sent_requests.push_back(*request);
    }
    cv_.notify_all();
    return next_return;
  }

  bool WaitForSentRequestCount(size_t count,
                               std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lk(mutex_);
    return cv_.wait_for(lk, timeout,
                        [&] { return sent_requests.size() >= count; });
  }

  std::vector<int64_t> SentNodeIds() {
    std::lock_guard<std::mutex> lk(mutex_);
    return sent_node_ids;
  }

  std::vector<Request> SentRequests() {
    std::lock_guard<std::mutex> lk(mutex_);
    return sent_requests;
  }

  int next_return = 0;
  std::vector<int64_t> sent_node_ids;
  std::vector<Request> sent_requests;

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
};

class BlockingResponseTransactionManager : public TransactionManager {
 public:
  explicit BlockingResponseTransactionManager(std::string response_payload)
      : TransactionManager(/*is_out_of_order=*/false,
                           /*need_response=*/true),
        response_payload_(std::move(response_payload)) {}

  std::unique_ptr<BatchUserResponse> ExecuteBatch(
      const BatchUserRequest& request) override {
    (void)request;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      execution_started_ = true;
      cv_.notify_all();
    }
    {
      std::unique_lock<std::mutex> lk(mutex_);
      cv_.wait(lk, [&] { return release_execution_; });
    }
    auto response = std::make_unique<BatchUserResponse>();
    response->add_response(response_payload_);
    return response;
  }

  bool WaitForExecutionStarted(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lk(mutex_);
    return cv_.wait_for(lk, timeout, [&] { return execution_started_; });
  }

  void ReleaseExecution() {
    std::lock_guard<std::mutex> lk(mutex_);
    release_execution_ = true;
    cv_.notify_all();
  }

  int RollbackToCheckpoint(uint64_t checkpoint_seq) override {
    std::lock_guard<std::mutex> lk(mutex_);
    rollback_checkpoint_ = checkpoint_seq;
    rollback_count_++;
    return 0;
  }

  uint64_t rollback_checkpoint() const { return rollback_checkpoint_; }

  int rollback_count() const { return rollback_count_; }

 private:
  std::string response_payload_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool execution_started_ = false;
  bool release_execution_ = false;
  uint64_t rollback_checkpoint_ = 0;
  int rollback_count_ = 0;
};

class TestSignatureVerifier : public SignatureVerifier {
 public:
  explicit TestSignatureVerifier(int64_t node_id)
      : SignatureVerifier(KeyInfo(), CertificateInfo()), node_id_(node_id) {}

  absl::StatusOr<SignatureInfo> SignMessage(
      const std::string& message) override {
    return absl::StatusOr<SignatureInfo>(MakeSignature(node_id_, message));
  }

  bool VerifyMessage(const std::string& message,
                     const SignatureInfo& sign) override {
    return sign.node_id() > 0 &&
           sign.signature() == "signed:" + message;
  }

  static SignatureInfo MakeSignature(int64_t node_id,
                                     const std::string& message) {
    SignatureInfo signature;
    signature.set_node_id(node_id);
    signature.set_signature("signed:" + message);
    return signature;
  }

 private:
  int64_t node_id_;
};

void AppendTestField(std::string* output, const std::string& value) {
  output->append(std::to_string(value.size()));
  output->push_back(':');
  output->append(value);
  output->push_back(';');
}

void AppendTestField(std::string* output, uint64_t value) {
  AppendTestField(output, std::to_string(value));
}

void AppendTestField(std::string* output, uint32_t value) {
  AppendTestField(output, std::to_string(value));
}

std::unique_ptr<Context> MakeSignedContext(int64_t node_id = 1) {
  auto context = std::make_unique<Context>();
  context->signature.set_signature("test-signature");
  context->signature.set_node_id(node_id);
  return context;
}

Request MakeGlobalCommittedRequest() {
  Request request;
  request.set_type(Request::TYPE_3PC_GLOBAL_COMMIT);
  request.set_seq(10);
  request.set_hash("hash-10");
  request.set_proxy_id(9);
  request.set_global_txn_id(123);
  request.set_coordinator_shard_id(0);
  request.set_global_coordinator_id(1);
  return request;
}

Request MakePOEExecuteRequest() {
  Request request = MakeGlobalCommittedRequest();
  request.set_type(Request::TYPE_POE_EXECUTE);
  request.set_sender_id(1);
  request.set_primary_id(1);
  request.set_current_view(1);
  request.set_local_shard_id(0);
  request.set_is_local_shard_poe(true);
  return request;
}

std::string BuildTestProofDigest(const Request& request,
                                 const std::string& result_digest) {
  std::string proof_payload;
  AppendTestField(&proof_payload, request.seq());
  AppendTestField(&proof_payload, request.hash());
  AppendTestField(&proof_payload, request.global_txn_id());
  AppendTestField(&proof_payload, request.coordinator_shard_id());
  AppendTestField(&proof_payload, request.local_shard_id());
  AppendTestField(&proof_payload, result_digest);
  return SignatureVerifier::CalculateHash(proof_payload);
}

std::string BuildTestRollbackDigest(uint64_t checkpoint_seq,
                                    uint32_t local_shard_id,
                                    const std::string& reason) {
  std::string rollback_payload;
  AppendTestField(&rollback_payload, checkpoint_seq);
  AppendTestField(&rollback_payload, local_shard_id);
  AppendTestField(&rollback_payload, reason);
  return SignatureVerifier::CalculateHash(rollback_payload);
}

Request MakePOEProofRequest(int64_t sender_id,
                            const std::string& result_digest = "result") {
  Request proof = MakePOEExecuteRequest();
  proof.set_type(Request::TYPE_POE_PROOF);
  proof.set_sender_id(sender_id);
  proof.set_data(result_digest);
  proof.set_data_hash(BuildTestProofDigest(proof, result_digest));
  *proof.mutable_data_signature() =
      TestSignatureVerifier::MakeSignature(sender_id, proof.data_hash());
  return proof;
}

Request MakePOERollbackRequest(uint64_t checkpoint_seq = 0,
                               const std::string& reason = "test rollback") {
  Request rollback;
  rollback.set_type(Request::TYPE_POE_ROLLBACK);
  rollback.set_seq(checkpoint_seq);
  rollback.set_sender_id(1);
  rollback.set_primary_id(1);
  rollback.set_current_view(1);
  rollback.set_local_shard_id(0);
  rollback.set_is_local_shard_poe(true);
  rollback.set_data(reason);
  rollback.set_data_hash(
      BuildTestRollbackDigest(checkpoint_seq, 0, reason));
  *rollback.mutable_data_signature() =
      TestSignatureVerifier::MakeSignature(1, rollback.data_hash());
  return rollback;
}

Request MakePOECertRequest(const std::vector<int64_t>& proof_senders,
                           const std::string& result_digest = "result") {
  Request cert = MakePOEProofRequest(/*sender_id=*/2, result_digest);
  cert.set_type(Request::TYPE_POE_CERT);
  cert.set_sender_id(1);
  cert.set_primary_id(1);
  cert.mutable_committed_certs()->Clear();
  for (int64_t proof_sender : proof_senders) {
    *cert.mutable_committed_certs()->add_committed_certs() =
        TestSignatureVerifier::MakeSignature(proof_sender, cert.data_hash());
  }
  *cert.mutable_data_signature() =
      TestSignatureVerifier::MakeSignature(/*node_id=*/1, cert.data_hash());
  return cert;
}

std::unique_ptr<BatchUserResponse> WaitForResponse(MessageManager* manager) {
  for (int i = 0; i < 20; ++i) {
    auto response = manager->GetResponseMsg();
    if (response != nullptr) {
      return response;
    }
  }
  return nullptr;
}

class ShardPOECommitmentHarness {
 public:
  explicit ShardPOECommitmentHarness(
      int64_t self_id,
      std::unique_ptr<TransactionManager> transaction_manager = nullptr,
      bool start_response_thread = false)
      : config_(GenerateConfig(self_id)),
        system_info_(config_),
        verifier_(self_id),
        checkpoint_manager_(config_, &replica_communicator_, &verifier_,
                            &system_info_),
        message_manager_(std::make_unique<MessageManager>(
            config_, std::move(transaction_manager), &checkpoint_manager_,
            &system_info_)),
        shard_metadata_(WriteShardConfig(), config_.GetSelfInfo().id()),
        shard_communicator_(&replica_communicator_, &shard_metadata_,
                            config_.GetReplicaInfos()),
        commitment_(config_, message_manager_.get(), &replica_communicator_,
                    &shard_communicator_, &shard_metadata_, &verifier_,
                    start_response_thread) {}

  ResDBConfig config_;
  SystemInfo system_info_;
  RecordingReplicaCommunicator replica_communicator_;
  TestSignatureVerifier verifier_;
  CheckPointManager checkpoint_manager_;
  std::unique_ptr<MessageManager> message_manager_;
  ShardMetadata shard_metadata_;
  ShardCommunicator shard_communicator_;
  ShardPOECommitment commitment_;
};

TEST(ShardPOECommitmentTest, LeaderBroadcastsLocalPOEExecute) {
  ShardPOECommitmentHarness harness(/*self_id=*/1);
  Request global_request = MakeGlobalCommittedRequest();

  EXPECT_EQ(harness.commitment_.ProcessGlobalCommittedRequest(global_request),
            0);
  EXPECT_EQ(harness.replica_communicator_.SentNodeIds(),
            (std::vector<int64_t>{1, 2, 3, 4}));
  auto sent_requests = harness.replica_communicator_.SentRequests();
  ASSERT_EQ(sent_requests.size(), 4);
  const Request& local_request = sent_requests.front();
  EXPECT_EQ(local_request.type(), Request::TYPE_POE_EXECUTE);
  EXPECT_EQ(local_request.local_shard_id(), 0);
  EXPECT_TRUE(local_request.is_local_shard_poe());
  EXPECT_EQ(local_request.sender_id(), 1);
  EXPECT_EQ(local_request.primary_id(), 1);
  EXPECT_EQ(local_request.seq(), global_request.seq());
  EXPECT_EQ(local_request.hash(), global_request.hash());
  EXPECT_EQ(local_request.proxy_id(), global_request.proxy_id());
  EXPECT_EQ(local_request.global_txn_id(), global_request.global_txn_id());
  EXPECT_EQ(local_request.coordinator_shard_id(),
            global_request.coordinator_shard_id());
  EXPECT_EQ(local_request.global_coordinator_id(),
            global_request.global_coordinator_id());
}

TEST(ShardPOECommitmentTest, NonLeaderDoesNotStartLocalPOE) {
  ShardPOECommitmentHarness harness(/*self_id=*/2);
  Request global_request = MakeGlobalCommittedRequest();

  EXPECT_EQ(harness.commitment_.ProcessGlobalCommittedRequest(global_request),
            0);
  EXPECT_TRUE(harness.replica_communicator_.SentNodeIds().empty());
}

TEST(ShardPOECommitmentTest, AcceptsValidPOEExecute) {
  ShardPOECommitmentHarness harness(/*self_id=*/2);
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            0);
}

TEST(ShardPOECommitmentTest, RejectsInvalidSender) {
  ShardPOECommitmentHarness harness(/*self_id=*/2);
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());
  request->set_sender_id(2);

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            -2);
  EXPECT_TRUE(harness.replica_communicator_.SentRequests().empty());
}

TEST(ShardPOECommitmentTest, RejectsWrongShard) {
  ShardPOECommitmentHarness harness(/*self_id=*/2);
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());
  request->set_local_shard_id(1);

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            -2);
  EXPECT_TRUE(harness.replica_communicator_.SentRequests().empty());
}

TEST(ShardPOECommitmentTest, RejectsMissingPOEMarker) {
  ShardPOECommitmentHarness harness(/*self_id=*/2);
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());
  request->set_is_local_shard_poe(false);

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            -2);
  EXPECT_TRUE(harness.replica_communicator_.SentRequests().empty());
}

TEST(ShardPOECommitmentTest, SendsProofOnlyAfterExecutionCompletes) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("proof-payload");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            0);
  const bool execution_started = transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000));
  EXPECT_TRUE(execution_started);
  if (execution_started) {
    EXPECT_FALSE(harness.replica_communicator_.WaitForSentRequestCount(
        1, std::chrono::milliseconds(50)));
  }

  transaction_manager_ptr->ReleaseExecution();
  ASSERT_TRUE(execution_started);
  ASSERT_TRUE(harness.replica_communicator_.WaitForSentRequestCount(
      1, std::chrono::milliseconds(1000)));

  EXPECT_EQ(harness.replica_communicator_.SentNodeIds(),
            (std::vector<int64_t>{1}));
  auto sent_requests = harness.replica_communicator_.SentRequests();
  ASSERT_EQ(sent_requests.size(), 1);
  const Request& proof = sent_requests.front();
  EXPECT_EQ(proof.type(), Request::TYPE_POE_PROOF);
  EXPECT_EQ(proof.seq(), 10);
  EXPECT_EQ(proof.hash(), "hash-10");
  EXPECT_EQ(proof.proxy_id(), 9);
  EXPECT_EQ(proof.global_txn_id(), 123);
  EXPECT_EQ(proof.coordinator_shard_id(), 0);
  EXPECT_EQ(proof.global_coordinator_id(), 1);
  EXPECT_EQ(proof.local_shard_id(), 0);
  EXPECT_EQ(proof.sender_id(), 2);
  EXPECT_EQ(proof.primary_id(), 1);
  EXPECT_TRUE(proof.is_local_shard_poe());
  EXPECT_FALSE(proof.data().empty());
  EXPECT_FALSE(proof.data_hash().empty());
  ASSERT_TRUE(proof.has_data_signature());
  EXPECT_EQ(proof.data_signature().node_id(), 2);
  EXPECT_EQ(proof.data_signature().signature(), "signed:" + proof.data_hash());
  EXPECT_EQ(harness.message_manager_->GetResponseMsg(), nullptr);
}

std::string RunProofAndReturnDigest(const std::string& response_payload) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>(response_payload);
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  auto context = MakeSignedContext();
  auto request = std::make_unique<Request>(MakePOEExecuteRequest());

  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(std::move(context),
                                                     std::move(request)),
            0);
  const bool execution_started = transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000));
  EXPECT_TRUE(execution_started);
  transaction_manager_ptr->ReleaseExecution();
  if (!execution_started) {
    return std::string();
  }
  EXPECT_TRUE(harness.replica_communicator_.WaitForSentRequestCount(
      1, std::chrono::milliseconds(1000)));
  auto sent_requests = harness.replica_communicator_.SentRequests();
  EXPECT_EQ(sent_requests.size(), 1);
  return sent_requests.empty() ? std::string()
                               : sent_requests.front().data_hash();
}

TEST(ShardPOECommitmentTest, ProofDigestChangesWhenResponseChanges) {
  const std::string first_digest = RunProofAndReturnDigest("response-a");
  const std::string second_digest = RunProofAndReturnDigest("response-b");

  EXPECT_FALSE(first_digest.empty());
  EXPECT_FALSE(second_digest.empty());
  EXPECT_NE(first_digest, second_digest);
}

TEST(ShardPOECommitmentTest, MatchingProofsCreatePOECert) {
  ShardPOECommitmentHarness harness(/*self_id=*/1);

  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(MakePOEProofRequest(1))),
            0);
  EXPECT_EQ(harness.replica_communicator_.SentRequests().size(), 0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2))),
            0);
  EXPECT_EQ(harness.replica_communicator_.SentRequests().size(), 0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(3),
                std::make_unique<Request>(MakePOEProofRequest(3))),
            0);

  auto sent_requests = harness.replica_communicator_.SentRequests();
  ASSERT_EQ(sent_requests.size(), 4);
  for (const auto& cert : sent_requests) {
    EXPECT_EQ(cert.type(), Request::TYPE_POE_CERT);
    EXPECT_EQ(cert.seq(), 10);
    EXPECT_EQ(cert.hash(), "hash-10");
    EXPECT_EQ(cert.committed_certs().committed_certs_size(), 3);
    EXPECT_TRUE(cert.has_data_signature());
    EXPECT_EQ(cert.data_signature().node_id(), 1);
  }
}

TEST(ShardPOECommitmentTest, DuplicateProofDoesNotIncreaseProofCount) {
  ShardPOECommitmentHarness harness(/*self_id=*/1);

  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2))),
            0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2))),
            0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(3),
                std::make_unique<Request>(MakePOEProofRequest(3))),
            0);

  EXPECT_EQ(harness.replica_communicator_.SentRequests().size(), 0);
}

TEST(ShardPOECommitmentTest, MismatchedProofDigestsDoNotCertify) {
  ShardPOECommitmentHarness harness(/*self_id=*/1);

  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2, "result-a"))),
            0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(3),
                std::make_unique<Request>(MakePOEProofRequest(3, "result-b"))),
            0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(4),
                std::make_unique<Request>(MakePOEProofRequest(4, "result-b"))),
            0);

  EXPECT_EQ(harness.replica_communicator_.SentRequests().size(), 0);
}

TEST(ShardPOECommitmentTest, RejectsInvalidPOEProofs) {
  ShardPOECommitmentHarness harness(/*self_id=*/1);

  Request bad_signature = MakePOEProofRequest(2);
  bad_signature.mutable_data_signature()->set_signature("bad");
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(bad_signature)),
            -2);

  Request wrong_shard = MakePOEProofRequest(2);
  wrong_shard.set_local_shard_id(1);
  wrong_shard.set_data_hash(BuildTestProofDigest(wrong_shard,
                                                 wrong_shard.data()));
  *wrong_shard.mutable_data_signature() =
      TestSignatureVerifier::MakeSignature(2, wrong_shard.data_hash());
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(wrong_shard)),
            -2);

  Request non_local_sender = MakePOEProofRequest(5);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(5),
                std::make_unique<Request>(non_local_sender)),
            -2);

  Request missing_marker = MakePOEProofRequest(2);
  missing_marker.set_is_local_shard_poe(false);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(missing_marker)),
            -2);
  EXPECT_EQ(harness.replica_communicator_.SentRequests().size(), 0);
}

TEST(ShardPOECommitmentTest, ValidCertReleasesHeldResponse) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("held-response");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(
                MakeSignedContext(),
                std::make_unique<Request>(MakePOEExecuteRequest())),
            0);
  ASSERT_TRUE(transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000)));
  transaction_manager_ptr->ReleaseExecution();
  ASSERT_TRUE(harness.replica_communicator_.WaitForSentRequestCount(
      1, std::chrono::milliseconds(1000)));
  EXPECT_EQ(harness.message_manager_->GetResponseMsg(), nullptr);

  Request cert = MakePOECertRequest({1, 2, 3});
  EXPECT_EQ(harness.commitment_.ProcessPOECertMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(cert)),
            0);

  auto response = WaitForResponse(harness.message_manager_.get());
  ASSERT_NE(response, nullptr);
  ASSERT_EQ(response->response_size(), 1);
  EXPECT_EQ(response->response(0), "held-response");
}

TEST(ShardPOECommitmentTest, CertBeforeExecutionCompletionReleasesLater) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("late-response");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(
                MakeSignedContext(),
                std::make_unique<Request>(MakePOEExecuteRequest())),
            0);
  ASSERT_TRUE(transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000)));

  Request cert = MakePOECertRequest({1, 2, 3});
  EXPECT_EQ(harness.commitment_.ProcessPOECertMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(cert)),
            0);
  transaction_manager_ptr->ReleaseExecution();

  auto response = WaitForResponse(harness.message_manager_.get());
  ASSERT_NE(response, nullptr);
  ASSERT_EQ(response->response_size(), 1);
  EXPECT_EQ(response->response(0), "late-response");
}

TEST(ShardPOECommitmentTest, InvalidCertDoesNotReleaseHeldResponse) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("held-response");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(
                MakeSignedContext(),
                std::make_unique<Request>(MakePOEExecuteRequest())),
            0);
  ASSERT_TRUE(transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000)));
  transaction_manager_ptr->ReleaseExecution();
  ASSERT_TRUE(harness.replica_communicator_.WaitForSentRequestCount(
      1, std::chrono::milliseconds(1000)));

  Request cert = MakePOECertRequest({1, 2, 3});
  cert.mutable_data_signature()->set_signature("bad");
  EXPECT_EQ(harness.commitment_.ProcessPOECertMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(cert)),
            -2);
  EXPECT_EQ(harness.message_manager_->GetResponseMsg(), nullptr);
}

TEST(ShardPOECommitmentTest, ValidRollbackFromLeaderResetsLocalState) {
  // A valid local leader rollback should propagate through MessageManager and
  // reset ordering to checkpoint_seq + 1.
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("unused");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));

  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(MakePOERollbackRequest(5))),
            0);

  EXPECT_EQ(transaction_manager_ptr->rollback_count(), 1);
  EXPECT_EQ(transaction_manager_ptr->rollback_checkpoint(), 5);
  EXPECT_EQ(harness.message_manager_->GetNextSeq(), 6);
}

TEST(ShardPOECommitmentTest, RejectsInvalidRollbackMessages) {
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("unused");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));

  Request bad_sender = MakePOERollbackRequest();
  bad_sender.set_sender_id(2);
  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(bad_sender)),
            -2);

  Request wrong_shard = MakePOERollbackRequest();
  wrong_shard.set_local_shard_id(1);
  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(wrong_shard)),
            -2);

  Request missing_marker = MakePOERollbackRequest();
  missing_marker.set_is_local_shard_poe(false);
  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(missing_marker)),
            -2);

  Request bad_signature = MakePOERollbackRequest();
  bad_signature.mutable_data_signature()->set_signature("bad");
  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(bad_signature)),
            -2);

  EXPECT_EQ(transaction_manager_ptr->rollback_count(), 0);
}

TEST(ShardPOECommitmentTest, ConflictingProofBroadcastsRollback) {
  // A same-sender proof conflict is the minimal Phase 5 rollback trigger. The
  // leader broadcasts TYPE_POE_ROLLBACK and applies it locally.
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("unused");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/1,
                                    std::move(transaction_manager));

  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2, "result-a"))),
            0);
  EXPECT_EQ(harness.commitment_.ProcessPOEProofMsg(
                MakeSignedContext(2),
                std::make_unique<Request>(MakePOEProofRequest(2, "result-b"))),
            4);

  EXPECT_EQ(transaction_manager_ptr->rollback_count(), 1);
  EXPECT_EQ(transaction_manager_ptr->rollback_checkpoint(), 0);

  auto sent_requests = harness.replica_communicator_.SentRequests();
  ASSERT_EQ(sent_requests.size(), 4);
  for (const auto& rollback : sent_requests) {
    EXPECT_EQ(rollback.type(), Request::TYPE_POE_ROLLBACK);
    EXPECT_EQ(rollback.seq(), 0);
    EXPECT_EQ(rollback.local_shard_id(), 0);
    EXPECT_TRUE(rollback.is_local_shard_poe());
    EXPECT_EQ(rollback.data(), "conflicting POE proof digest");
  }
}

TEST(ShardPOECommitmentTest, RollbackSuppressesOldHeldResponseRelease) {
  // Once rollback drops a held response, a delayed certificate must not release
  // it to the client/proxy.
  auto transaction_manager =
      std::make_unique<BlockingResponseTransactionManager>("held-response");
  auto* transaction_manager_ptr = transaction_manager.get();
  ShardPOECommitmentHarness harness(/*self_id=*/2,
                                    std::move(transaction_manager));
  EXPECT_EQ(harness.commitment_.ProcessPOEExecuteMsg(
                MakeSignedContext(),
                std::make_unique<Request>(MakePOEExecuteRequest())),
            0);
  ASSERT_TRUE(transaction_manager_ptr->WaitForExecutionStarted(
      std::chrono::milliseconds(1000)));
  transaction_manager_ptr->ReleaseExecution();
  ASSERT_TRUE(harness.replica_communicator_.WaitForSentRequestCount(
      1, std::chrono::milliseconds(1000)));

  EXPECT_EQ(harness.commitment_.ProcessPOERollbackMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(MakePOERollbackRequest(0))),
            0);
  EXPECT_EQ(transaction_manager_ptr->rollback_count(), 1);

  Request cert = MakePOECertRequest({1, 2, 3});
  EXPECT_EQ(harness.commitment_.ProcessPOECertMsg(
                MakeSignedContext(1),
                std::make_unique<Request>(cert)),
            0);
  EXPECT_EQ(harness.message_manager_->GetResponseMsg(), nullptr);
}

}  // namespace
}  // namespace resdb
