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

#include "platform/consensus/ordering/hotstuff/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace hotstuff {

Commitment::Commitment(const ResDBConfig& config,
                       MessageManager* message_manager,
                       ReplicaCommunicator* replica_communicator,
                       SignatureVerifier* verifier)
    : CommitmentBasic(config, message_manager, replica_communicator, verifier),
      message_manager_(message_manager) {
  current_view_ = 0;
}

Commitment::~Commitment() {}

void Commitment::Init() {
  current_view_ = 0;
  SendNewView();
}

QC Commitment::GetQC(int64_t view_num, HotStuffRequest::Type type) {
  QC qc;
  qc.set_type(type);
  for (const auto& request : received_requests_[view_num % 128][type]) {
    *qc.mutable_node() = request->node();
    *qc.add_signatures() = request->node_signature();
  }
  return qc;
}

QC Commitment::GetHighQC(int64_t view_num) {
  QC high_qc;
  high_qc.set_type(HotStuffRequest::TYPE_PRECOMMIT);
  for (const auto& request :
       received_requests_[view_num % 128][HotStuffRequest::TYPE_PREPARE_VOTE]) {
    if (request->node().info().view() > high_qc.node().info().view()) {
      *high_qc.mutable_node() = request->node();
    }
    *high_qc.add_signatures() = request->node_signature();
  }
  return high_qc;
}

HotStuffRequest::Type Commitment::GetNextState(int type) {
  switch (type) {
    case HotStuffRequest::TYPE_PREPARE:
    case HotStuffRequest::TYPE_PREPARE_VOTE:
      return HotStuffRequest::TYPE_PRECOMMIT;
    case HotStuffRequest::TYPE_PRECOMMIT:
    case HotStuffRequest::TYPE_PRECOMMIT_VOTE:
      return HotStuffRequest::TYPE_COMMIT;
    case HotStuffRequest::TYPE_COMMIT:
    case HotStuffRequest::TYPE_COMMIT_VOTE:
      return HotStuffRequest::TYPE_DECIDE;
    case HotStuffRequest::TYPE_DECIDE:
      return HotStuffRequest::TYPE_NEWVIEW;
    case HotStuffRequest::TYPE_NONE:
      return HotStuffRequest::TYPE_NEWVIEW;
    default:
      return HotStuffRequest::TYPE_NONE;
  }
}

HotStuffRequest::Type Commitment::VoteType(int type) {
  switch (type) {
    case HotStuffRequest::TYPE_PREPARE:
      return HotStuffRequest::TYPE_PREPARE_VOTE;
    case HotStuffRequest::TYPE_PRECOMMIT:
      return HotStuffRequest::TYPE_PRECOMMIT_VOTE;
    case HotStuffRequest::TYPE_COMMIT:
      return HotStuffRequest::TYPE_COMMIT_VOTE;
    default:
      return HotStuffRequest::TYPE_NONE;
  }
}

std::unique_ptr<Request> Commitment::NewRequest(const HotStuffRequest& request,
                                                HotStuffRequest::Type type) {
  HotStuffRequest new_request(request);
  if (type != HotStuffRequest::TYPE_NONE) {
    new_request.set_type(type);
  } else {
    HotStuffRequest::Type next_type = GetNextState(request.type());
    new_request.set_type(next_type);
  }
  new_request.set_sender_id(id_);
  auto ret = std::make_unique<Request>();
  new_request.SerializeToString(ret->mutable_data());
  return ret;
}

class Timer {
 public:
  Timer(std::string name) {
    start_ = GetCurrentTime();
    name_ = name;
  }
  ~Timer() {
    // LOG(ERROR) << name_ << " run time:" << (GetCurrentTime() - start_);
  }

 private:
  uint64_t start_;
  std::string name_;
};

// 1. Client sends requests to the primary. Primary broadcasts a prepare
// message.
// 2. each replica responses a prepare vote.
// 3. Primary receives 2f+1 votes for prepare, sends out a pre-commit message.
int Commitment::Process(std::unique_ptr<HotStuffRequest> request) {
  Timer timer(HotStuffRequest_Type_Name(request->type()));
  switch (request->type()) {
    case HotStuffRequest::TYPE_NEWREQUEST:
      return ProcessNewRequest(std::move(request));
    case HotStuffRequest::TYPE_PREPARE_VOTE:
    case HotStuffRequest::TYPE_PRECOMMIT_VOTE:
    case HotStuffRequest::TYPE_COMMIT_VOTE:
    case HotStuffRequest::TYPE_NEWVIEW: {
      return ProcessMessageOnPrimary(std::move(request));
    }
    case HotStuffRequest::TYPE_PREPARE:
    case HotStuffRequest::TYPE_PRECOMMIT:
    case HotStuffRequest::TYPE_COMMIT:
    case HotStuffRequest::TYPE_DECIDE: {
      std::unique_ptr<HotStuffRequest> user_request = nullptr;
      if (request->type() == HotStuffRequest::TYPE_PRECOMMIT) {
        if (request->has_new_user_request()) {
          user_request =
              std::make_unique<HotStuffRequest>(request->new_user_request());
        }
      }
      ProcessMessageOnReplica(std::move(request));
      if (user_request) {
        ProcessMessageOnReplica(std::move(user_request));
      }
      return 0;
    }
    default:
      return -2;
  }
}

int Commitment::ProcessNewRequest(std::unique_ptr<HotStuffRequest> request) {
  request->mutable_node()->mutable_info()->set_hash(
      SignatureVerifier::CalculateHash(request->node().data()));
  global_stats_->IncClientRequest();
  request_list_.Push(std::move(request));
  return 0;
}

std::unique_ptr<HotStuffRequest> Commitment::GetClientRequest() {
  while (!stop_) {
    auto request = request_list_.Pop(config_.ClientBatchWaitTimeMS());
    if (request == nullptr) {
      continue;
    }
    return request;
  }
  return nullptr;
}

// ========= Start function ===============
void Commitment::SendNewView() {
  QC prepare_qc = message_manager_->GetPrepareQC();
  auto user_request = std::make_unique<HotStuffRequest>();
  *user_request->mutable_qc() = prepare_qc;
  user_request->set_view(current_view_);
  std::unique_ptr<Request> vote_request =
      NewRequest(*user_request, HotStuffRequest::TYPE_NEWVIEW);
  current_view_++;
  // LOG(ERROR) << "send new msg view:" << current_view_<<" to
  // primary:"<<PrimaryId(current_view_); send vote to the primary.
  replica_communicator_->SendMessage(*vote_request, PrimaryId(current_view_));
}

// 1. Obtain the highQC from 2f+1 new view messages.
// 2. Create a leave on the hightQC.
// 3. Broadcast a prepare message.
int Commitment::ProcessNewView(int64_t view_number) {
  if (PrimaryId(view_number) != id_) {
    LOG(ERROR) << "not current primary:" << view_number;
    return -2;
  }
  auto user_request = GetClientRequest();
  if (user_request == nullptr) {
    LOG(ERROR) << "data is empty";
    return -2;
  }
  QC high_qc = GetHighQC(view_number - 1);
  // LOG(ERROR) << "receive new view. view number:" << view_number;
  // LOG(ERROR) << "high qc:" << high_qc.node().info().view()<<"
  // hash:"<<high_qc.node().info().hash();
  *user_request->mutable_node()->mutable_pre() = high_qc.node().info();
  user_request->mutable_node()->mutable_info()->set_view(view_number);
  *user_request->mutable_qc() = high_qc;

  std::unique_ptr<Request> new_request =
      NewRequest(*user_request, HotStuffRequest::TYPE_PREPARE);
  current_view_ = view_number;
  // LOG(ERROR) << "new view start:" << current_view_;
  replica_communicator_->BroadCast(*new_request);
  return 0;
}

std::unique_ptr<HotStuffRequest> Commitment::GetNewViewMessage(
    int64_t view_number) {
  if (PrimaryId(view_number) != id_) {
    LOG(ERROR) << "not current primary:" << view_number;
    return nullptr;
  }
  QC high_qc = GetHighQC(view_number - 1);
  auto user_request = GetClientRequest();
  if (user_request == nullptr) {
    LOG(ERROR) << "data is empty";
    return nullptr;
  }
  *user_request->mutable_node()->mutable_pre() = high_qc.node().info();
  user_request->mutable_node()->mutable_info()->set_view(view_number);
  user_request->set_view(view_number);
  *user_request->mutable_qc() = high_qc;
  user_request->set_type(HotStuffRequest::TYPE_PREPARE);
  return user_request;
}

bool Commitment::VerifyNodeSignagure(const HotStuffRequest& request) {
  if (verifier_ == nullptr) {
    return true;
  }

  if (!request.has_node_signature()) {
    LOG(ERROR) << "node signature is empty";
    return false;
  }
  std::string data;
  request.node().SerializeToString(&data);
  // check signatures
  bool valid = verifier_->VerifyMessage(data, request.node_signature());
  if (!valid) {
    LOG(ERROR) << "signature is not valid:"
               << request.node_signature().DebugString();
    return false;
  }
  return true;
}

bool Commitment::VerifyTS(const HotStuffRequest& request) {
  if (verifier_ == nullptr) {
    return true;
  }
  if (request.qc().signatures_size() < config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "signature is not enough:" << request.qc().signatures_size();
    return false;
  }
  std::string data;
  request.qc().node().SerializeToString(&data);
  for (const auto& signature : request.qc().signatures()) {
    bool valid = verifier_->VerifyMessage(data, signature);
    if (!valid) {
      LOG(ERROR) << "TS is not valid";
      return false;
    }
  }
  return true;
}

int Commitment::ProcessMessageOnPrimary(
    std::unique_ptr<HotStuffRequest> request) {
  HotStuffRequest::Type type = (HotStuffRequest::Type)request->type();
  // LOG(INFO) << "primary get type:" << HotStuffRequest_Type_Name(type)
  //           << " view:" << request->node().info().view()
  //           << " from:" << request->sender_id();
  if (type != HotStuffRequest::TYPE_NEWVIEW) {
    if (!VerifyNodeSignagure(*request)) {
      LOG(ERROR) << "signature invalid";
      return -2;
    }
  } else {
    if (PrimaryId(request->view() + 1) != id_) {
      LOG(ERROR) << "not current primary:" << request->view() + 1;
      return -2;
    }
  }

  int64_t view_num = request->view();
  int64_t node_view = request->node().info().view();
  int64_t qc_view = request->qc().node().info().view();
  int32_t sender_id = request->sender_id();
  int receive_size = 0;
  if (view_num < current_view_ - 5) {
    LOG(ERROR) << " view :" << view_num
               << " is too old, current view:" << current_view_;
    return -2;
  }
  {
    std::unique_lock<std::mutex> lk(mutex_[view_num % 128]);
    auto ret =
        received_senders_[view_num % 128][type].insert(request->sender_id());
    if (!ret.second) {
      LOG(ERROR) << "sender:" << request->sender_id()
                 << " has been received. view:" << request->view();
      return -2;
    }
    received_requests_[view_num % 128][type].push_back(std::move(request));
    receive_size = received_senders_[view_num % 128][type].size();
  }

  {
    std::unique_lock<std::mutex> lk(mutex_[128]);
    if (type == HotStuffRequest::TYPE_PREPARE_VOTE) {
      current_view_ = std::max(current_view_, node_view);
    }
  }

  // LOG(ERROR) << "primary get size:" << receive_size
  //            << " type:" << HotStuffRequest_Type_Name(type)
  //            << " view:" << view_num << " qc view:" << qc_view
  //            << " from:" << sender_id << " node view:" << node_view;
  // if have received 2f+1 votes, broadcast next message.
  if (receive_size == config_.GetMinDataReceiveNum()) {
    if (type == HotStuffRequest::TYPE_NEWVIEW) {
      // Only for the first view to boost up the server.
      return ProcessNewView(view_num + 1);
    }
    // New node has been added in QC.
    HotStuffRequest new_hotstuff_request;
    new_hotstuff_request.set_type(type);
    new_hotstuff_request.set_view(view_num);
    *new_hotstuff_request.mutable_qc() = GetQC(view_num, type);

    if (type == HotStuffRequest::TYPE_PREPARE_VOTE &&
        view_num == current_view_) {
      //	    LOG(ERROR)<<"get new view :"<<view_num+1;
      auto new_user_request = GetNewViewMessage(view_num + 1);

      new_user_request->set_sender_id(id_);
      *new_hotstuff_request.mutable_new_user_request() = *new_user_request;

      std::unique_ptr<Request> new_request = NewRequest(new_hotstuff_request);
      replica_communicator_->BroadCast(*new_request);
    } else {
      std::unique_ptr<Request> new_request = NewRequest(new_hotstuff_request);
      replica_communicator_->BroadCast(*new_request);
    }
  }
  return 0;
}

int Commitment::ProcessMessageOnReplica(
    std::unique_ptr<HotStuffRequest> request) {
  // LOG(ERROR) << "Replica receive type:"
  //            << HotStuffRequest_Type_Name(request->type())
  //            << " node view:" << request->node().info().view()
  //            << " qc view:" << request->qc().node().info().view()
  //            << " from:" << request->sender_id();
  if (request->type() == HotStuffRequest::TYPE_PRECOMMIT) {
    global_stats_->IncPropose();
  }
  if (request->type() == HotStuffRequest::TYPE_PREPARE) {
    received_senders_[(request->node().info().view() + 64) % 128].clear();
    received_requests_[(request->node().info().view() + 64) % 128].clear();
    global_stats_->IncPrepare();
  }
  if (request->type() == HotStuffRequest::TYPE_COMMIT) {
    global_stats_->IncCommit();
  }
  if (request->type() == HotStuffRequest::TYPE_DECIDE) {
    global_stats_->IncExecute();
  }
  if (request->type() == HotStuffRequest::TYPE_PREPARE) {
    // node's pre is highQC
    if (request->node().pre().hash() != request->qc().node().info().hash()) {
      LOG(ERROR) << "high qc hash not same";
      return -2;
    }
    if (!message_manager_->IsSaveNode(*request)) {
      LOG(ERROR) << "not safe node";
      return -2;
    }
  } else {
    if (!VerifyTS(*request)) {
      LOG(ERROR) << " ts not invlid"
                 << " request type:"
                 << HotStuffRequest_Type_Name(request->type());
      return -2;
    }
  }

  // LOG(INFO) << "Replica receive type:"
  //           << HotStuffRequest_Type_Name(request->type())
  //           << " current view view:" << current_view_
  //           << " qc view:" << request->qc().node().info().view()
  //           << " from:" << request->sender_id();
  message_manager_->UpdateNode(*request);
  if (request->type() == HotStuffRequest::TYPE_DECIDE) {
    // execute and send new view
    message_manager_->Commit(std::move(request));
  } else {
    // For prepare, send back the new node in qc. Then every message in the
    // future, the new node will be included in QC.
    if (!request->has_node()) {
      *request->mutable_node() = request->qc().node();
    }
    request->set_view(request->node().info().view());

    if (verifier_) {
      std::string data;
      request->node().SerializeToString(&data);
      auto signature_or = verifier_->SignMessage(data);
      if (!signature_or.ok()) {
        LOG(ERROR) << "Sign message fail";
        return -2;
      }
      *request->mutable_node_signature() = *signature_or;
    }
    // For ChainHotStuff, forward the lockQC as genericQC
    if (request->type() == HotStuffRequest::TYPE_PREPARE) {
      *request->mutable_qc() = message_manager_->GetPrepareQC();
    }
    std::unique_ptr<Request> vote_request =
        NewRequest(*request, VoteType(request->type()));
    // send vote to the primary.
    // LOG(INFO) << "send type:"
    //           << HotStuffRequest_Type_Name(VoteType(request->type()))
    //           << " to:" << PrimaryId(request->sender_id() + 1) << " view"
    //           << request->node().info().view();
    replica_communicator_->SendMessage(*vote_request,
                                       PrimaryId(request->sender_id() + 1));
  }
  return 0;
}

}  // namespace hotstuff
}  // namespace resdb
