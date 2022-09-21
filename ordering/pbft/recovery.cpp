#include "ordering/pbft/recovery.h"

#include <unistd.h>

#include "crypto/signature_verifier.h"
#include "glog/logging.h"
#include "ordering/pbft/transaction_utils.h"

namespace resdb {

Recovery::Recovery(const ResDBConfig& config,
                   TransactionManager* transaction_manager,
                   ResDBReplicaClient* replica_client,
                   SignatureVerifier* verifier)
    : config_(config),
      transaction_manager_(transaction_manager),
      replica_client_(replica_client),
      verifier_(verifier),
      stop_(false) {
  if (config_.IsCheckPointEnabled()) {
    healthy_thread_ = std::thread(&Recovery::HealthCheck, this);
  }
}

Recovery::~Recovery() {
  stop_ = true;
  if (healthy_thread_.joinable()) {
    healthy_thread_.join();
  }
}

bool Recovery::IsPrimary() {
  return transaction_manager_->GetCurrentPrimary() ==
         config_.GetSelfInfo().id();
}

int Recovery::ProcessRecoveryData(std::unique_ptr<Context> context,
                                  std::unique_ptr<Request> request) {
  RecoveryRequest recovery_request;
  if (!recovery_request.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return -2;
  }
  if (request->sender_id() == config_.GetSelfInfo().id()) {
    return 0;
  }

  RequestSet data = transaction_manager_->GetRequestSet(
      recovery_request.min_seq(), recovery_request.max_seq());

  // Create a normal reqeust and ask for a checkpoint
  std::unique_ptr<Request> resp = NewRequest(
      Request::TYPE_RECOVERY_DATA_RESP, Request(), config_.GetSelfInfo().id());

  if (!data.SerializeToString(resp->mutable_data())) {
    return -1;
  }

  LOG(ERROR) << "resp for recovery:[" << recovery_request.min_seq() << ","
             << recovery_request.max_seq() << "]"
             << " sender:" << resp->sender_id();
  replica_client_->SendMessage(*resp, request->sender_id());
  return 0;
}

// TODO move to Recovery.
int Recovery::ProcessRecoveryDataResp(std::unique_ptr<Context> context,
                                      std::unique_ptr<Request> request) {
  RequestSet data;
  if (!data.ParseFromString(request->data())) {
    LOG(ERROR) << "parse data fail";
    return -2;
  }
  LOG(ERROR) << "receive recovery data size:" << data.requests_size();

  for (const RequestWithProof& recovery_request : data.requests()) {
    // should contain 2f+1 distinct responses.
    if (recovery_request.proofs_size() != config_.GetMinDataReceiveNum()) {
      break;
    }

    bool valid = true;
    std::set<int32_t> senders;
    const Request& request = recovery_request.request();
    if (request.seq() != recovery_request.seq()) {
      LOG(ERROR) << "seq not match, request seq:" << request.seq()
                 << "  recovery seq:" << recovery_request.seq();
      break;
    }
    for (const RequestWithProof::RequestData& sub_request :
         recovery_request.proofs()) {
      int32_t sender = sub_request.request().sender_id();
      // should from distinct replicas.
      if (senders.find(sender) != senders.end()) {
        LOG(ERROR) << "find the same sender:" << sender;
        valid = false;
        break;
      }
      // Check Hash
      if (sub_request.request().hash() != request.hash()) {
        LOG(ERROR) << "hash value not the same";
        valid = false;
        break;
      }
      // is valid messages.
      if (!verifier_->VerifyMessage(sub_request.request(),
                                    sub_request.signature())) {
        valid = false;
        break;
      }
    }
    if (valid) {
      transaction_manager_->CommittedRequestWithProof(recovery_request);
    }
  }

  return 0;
}

void Recovery::AddNewReplica(const ReplicaInfo& info) {
  if (!IsPrimary()) {
    return;
  }
  LOG(INFO) << " new replica is comming:" << info.DebugString();

  std::unique_ptr<Request> request = NewRequest(
      Request::TYPE_PRE_PREPARE, Request(), config_.GetSelfInfo().id());

  SystemInfoRequest system_info_request;
  system_info_request.set_type(SystemInfoRequest::ADD_REPLICA);
  NewReplicaRequest new_replica_request;
  *new_replica_request.mutable_replica_info() = info;

  if (!new_replica_request.SerializeToString(
          system_info_request.mutable_request())) {
    return;
  }

  if (!system_info_request.SerializeToString(request->mutable_data())) {
    return;
  }

  request->set_hash(SignatureVerifier::CalculateHash(request->data()));
  request->set_is_system_request(true);
  replica_client_->BroadCast(*request);
}

void Recovery::HealthCheck() {
  uint64_t last_current_executed_seq = 0;
  int healthy_check_period_microseconds = 10000000;
  if (config_.IsTestMode()) {
    healthy_check_period_microseconds = 1000000;
  }
  while (!stop_) {
    uint64_t stable_checkpoint_seq =
        transaction_manager_->GetStableCheckPointSeq();
    uint64_t current_executed_seq =
        transaction_manager_->GetMaxCheckPointRequestSeq();
    LOG(ERROR) << "current execut:" << current_executed_seq
               << " last:" << last_current_executed_seq;
    // If there is no any executed message for a long time or the stable seq
    // is larger than the local checkpoint, trigger a recovery.
    if ((last_current_executed_seq != 0 &&
         last_current_executed_seq == current_executed_seq) ||
        stable_checkpoint_seq > current_executed_seq) {
      LOG(ERROR) << "current stable executed seq:" << stable_checkpoint_seq
                 << " current executed seq:" << current_executed_seq
                 << " last:" << last_current_executed_seq << " need recovery";

      // Create a normal reqeust and ask for a checkpoint
      std::unique_ptr<Request> request = NewRequest(
          Request::TYPE_RECOVERY_DATA, Request(), config_.GetSelfInfo().id());

      RecoveryRequest recovery_request;
      // ask for committed requests with size
      recovery_request.set_max_seq(current_executed_seq +
                                   config_.GetCheckPointWaterMark());
      recovery_request.set_min_seq(current_executed_seq);

      if (!recovery_request.SerializeToString(request->mutable_data())) {
        continue;
      }

      request->set_is_system_request(true);
      replica_client_->BroadCast(*request);
    }
    last_current_executed_seq = current_executed_seq;
    std::this_thread::sleep_for(
        std::chrono::microseconds(healthy_check_period_microseconds));
  }
}

}  // namespace resdb
