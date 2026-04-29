#include "platform/consensus/ordering/achilles/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>
#include "common/utils/utils.h"

namespace resdb {
namespace achilles {

AchillesConsensus::AchillesConsensus(const ResDBConfig& config,
                                     std::unique_ptr<TransactionManager> executor,
                                     oe_enclave_t* enclave)
    : Consensus(config, std::move(executor)) {
  int total_replicas = config_.GetReplicaNum();
  // Achilles uses n=2f+1, so f = (n-1)/2
  int f = (total_replicas - 1) / 2;

  Init();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    achilles_ = std::make_unique<Achilles>(config_.GetSelfInfo().id(), f,
                                           total_replicas, GetSignatureVerifier(),
                                           config, enclave);

    achilles_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    achilles_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    achilles_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
  LOG(ERROR) << "Achilles consensus init done";
}

int AchillesConsensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (request->user_type() == MessageType::NewView) {
    auto vc = std::make_unique<ViewCertificate>();
    if (!vc->ParseFromString(request->data())) {
      LOG(ERROR) << "parse ViewCertificate fail";
      return -1;
    }
    achilles_->ReceiveViewCert(std::move(vc));
  } else if (request->user_type() == MessageType::Commit) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse Proposal fail";
      return -1;
    }
    achilles_->ReceiveProposal(std::move(proposal));
  } else if (request->user_type() == MessageType::StoreVote) {
    auto sc = std::make_unique<StoreCertificate>();
    if (!sc->ParseFromString(request->data())) {
      LOG(ERROR) << "parse StoreCertificate fail";
      return -1;
    }
    achilles_->ReceiveStoreCert(std::move(sc));
  } else if (request->user_type() == MessageType::Decide) {
    auto cc = std::make_unique<CommitmentCertificate>();
    if (!cc->ParseFromString(request->data())) {
      LOG(ERROR) << "parse CommitmentCertificate fail";
      return -1;
    }
    achilles_->ReceiveDecide(std::move(cc));
  }
  return 0;
}

int AchillesConsensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return achilles_->ReceiveTransaction(std::move(txn));
}

int AchillesConsensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int AchillesConsensus::CommitMsgInternal(const Transaction& txn) {
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace achilles
}  // namespace resdb
