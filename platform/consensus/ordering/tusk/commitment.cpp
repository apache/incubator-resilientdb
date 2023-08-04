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

#include "platform/consensus/ordering/tusk/commitment.h"

#include <glog/logging.h>
#include <unistd.h>

#include <stack>

#include "common/utils/utils.h"
#include "platform/consensus/ordering/tusk/tusk_utils.h"

namespace resdb {
namespace tusk {

Commitment::Commitment(const ResDBConfig& config,
                       MessageManager* message_manager,
                       ReplicaCommunicator* replica_communicator,
                       SignatureVerifier* verifier)
    : CommitmentBasic(config, message_manager, replica_communicator, verifier),
      message_manager_(message_manager),
      current_seq_(1),
      current_round_(0) {
  consensus_thread_ = std::thread(&Commitment::DAGConsensus, this);
}

Commitment::~Commitment() {
  stop_ = true;
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
}

int Commitment::GetLeader(int64_t r) {
  return r / 2 % config_.GetReplicaInfos().size() + 1;
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

int Commitment::Process(std::unique_ptr<TuskRequest> request) {
  Timer timer(TuskRequest_Type_Name(request->type()));
  // LOG(ERROR) << " get process type:" <<
  // TuskRequest_Type_Name(request->type())
  //           << " from:" << request->sender_id();
  switch (request->type()) {
    case TuskRequest::TYPE_NEWREQUEST:
    case TuskRequest::TYPE_NEW_BLOCK:
      return ProcessNewRequest(std::move(request));
    default:
      return -2;
  }
}

void Commitment::BroadCastBlock(const TuskRequest& block,
                                TuskRequest::Type type) {
  std::unique_ptr<Request> bc_request = NewRequest(block, type, id_);
  bc_request->set_type(TuskRequest::TYPE_BLOCK);
  replica_communicator_->BroadCast(*bc_request);
}

void Commitment::SendBlock(const TuskRequest& block, TuskRequest::Type type) {
  std::unique_ptr<Request> bc_request = NewRequest(block, type, id_);
  bc_request->set_type(TuskRequest::TYPE_BLOCK);
  replica_communicator_->SendMessage(*bc_request, block.sender_id());
}

void Commitment::BroadCastMetadata(const TuskBlockMetadata& metadata,
                                   TuskRequest::Type type) {
  std::unique_ptr<Request> bc_request = NewRequest(metadata, type, id_);
  bc_request->set_type(TuskRequest::TYPE_METADATA);
  replica_communicator_->BroadCast(*bc_request);
}

bool Commitment::CheckQC(const TuskRequest& request) {
  std::set<int> sig_owners;
  for (const auto& sig : request.qc().certificates()) {
    auto valid = verifier_->VerifyMessage(request.hash(), sig);
    if (!valid) {
      LOG(ERROR) << "certificate is not valid";
      return false;
    }
    auto ret = sig_owners.insert(sig.node_id());
    if (!ret.second) {
      LOG(ERROR) << " new block sig dupliated";
      return false;
    }
  }
  return true;
}

void Commitment::Sign(TuskRequest* request) {
  if (verifier_) {
    auto hash_signature_or = verifier_->SignMessage(request->hash());
    if (!hash_signature_or.ok()) {
      LOG(ERROR) << "Sign message fail";
      return;
    }
    *request->mutable_data_signature() = *hash_signature_or;
  }
}

bool Commitment::Verify(const TuskRequest& request) {
  if (verifier_) {
    bool valid =
        verifier_->VerifyMessage(request.hash(), request.data_signature());
    if (!valid) {
      LOG(ERROR) << "certificate is not valid";
      return false;
    }
    if (request.data_signature().node_id() != request.sender_id()) {
      LOG(ERROR) << " sender is not the same";
      return false;
    }
  }
  return true;
}

// sent from clients, new client request.
void Commitment::AddNewRequest(std::unique_ptr<TuskRequest> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  std::string hash = request->hash();
  new_request_.push(std::move(request));
  MayBroadCastBlock();
}

// sent from clients, new client request.
void Commitment::AddBlockACK(std::unique_ptr<TuskRequest> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (candidate_.find(request->hash()) == candidate_.end()) {
    // LOG(ERROR) << "hash not valid";
    return;
  }

  std::string hash = request->hash();
  int sender = request->sender_id();
  received_block_ack_[hash][sender] = std::move(request);
  // 2f+1 certificates
  if (received_block_ack_[hash].size() ==
      static_cast<size_t>(config_.GetMinDataReceiveNum())) {
    auto ready_request = std::move(candidate_[hash]);
    for (const auto& signatures : received_block_ack_[hash]) {
      *ready_request->mutable_qc()->add_certificates() =
          signatures.second->data_signature();
    }
    GenerateMetadata(*ready_request);
    candidate_.erase(candidate_.find(ready_request->hash()));
    received_block_ack_.erase(received_block_ack_.find(ready_request->hash()));
    ready_request_list_[ready_request->round()][ready_request->source_id()] =
        std::move(ready_request);
  }
}

// sent from others, including 2f+1 certificates.
void Commitment::AddReadyRequest(std::unique_ptr<TuskRequest> request) {
  std::unique_lock<std::mutex> lk(mutex_);
  // LOG(ERROR) << "!!!! add ready request:" << request->round()
  //           << " souce:" << request->source_id();
  for (const auto& metadata : request->strong_metadata()) {
    // LOG(ERROR)<<"get metad data:"<<metadata.round()<<"
    // source:"<<metadata.source_id();
    reference_[metadata.round()][metadata.source_id()].insert(
        request->source_id());
  }
  ready_request_list_[request->round()][request->source_id()] =
      std::move(request);
}

std::unique_ptr<TuskRequest> Commitment::GetRequest(int64_t round, int id) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_request_list_[round].find(id) == ready_request_list_[round].end()) {
    return nullptr;
  }
  if (ready_request_list_[round][id] == nullptr) {
    return nullptr;
  }
  return std::make_unique<TuskRequest>(*ready_request_list_[round][id]);
}

int Commitment::GetReferenceNum(const TuskRequest& req) {
  std::unique_lock<std::mutex> lk(mutex_);
  return reference_[req.round()][req.source_id()].size();
}

void Commitment::AddLeaderRound(int64_t round) {
  std::lock_guard<std::mutex> lk(ld_mutex_);
  leader_round_.push(round);
  ld_cv_.notify_all();
}

int64_t Commitment::GetLeaderRound() {
  int timeout_ms = 1000000;
  std::unique_lock<std::mutex> lk(ld_mutex_);
  ld_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms),
                  [&] { return !leader_round_.empty() || stop_; });
  if (leader_round_.empty()) {
    return -1;
  }
  int64_t round = leader_round_.front();
  leader_round_.pop();
  return round;
}

void Commitment::RemoveData(int64_t round, int id) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto& ready_map = ready_request_list_[round];
  auto sender_it = ready_map.find(id);
  if (sender_it == ready_map.end()) {
    return;
  }

  auto req = std::move(sender_it->second);
  ready_map.erase(sender_it);
  auto& reference_map = reference_[round];
  if (reference_map.find(id) == reference_map.end()) {
    // LOG(ERROR) << "no ref:";
  } else {
    reference_map.erase(reference_map.find(id));
  }
}

// ========= Create Block and Metadata ===============
int Commitment::ProcessNewRequest(std::unique_ptr<TuskRequest> request) {
  global_stats_->IncClientRequest();
  // LOG(ERROR) << "get block :" << TuskRequest_Type_Name(request->type());
  switch (request->type()) {
    case TuskRequest::TYPE_NEWREQUEST: {
      if (request->round() > 0) {
        if (request->strong_metadata_size() < config_.GetMinDataReceiveNum()) {
          LOG(ERROR) << " new block ont include enought metadata:"
                     << request->strong_metadata_size();
          return -2;
        }
        if (!CheckQC(*request)) {
          return -2;
        }
      }
      request->set_source_id(id_);
      AddNewRequest(std::move(request));
      break;
    }
    case TuskRequest::TYPE_NEW_BLOCK: {
      Sign(request.get());
      SendBlock(*request, TuskRequest::TYPE_BLOCK_ACK);
      AddReadyRequest(std::move(request));
      break;
    }
    case TuskRequest::TYPE_BLOCK_ACK: {
      if (!Verify(*request)) {
        return -2;
      }

      AddBlockACK(std::move(request));
    } break;
  }
  return 0;
}

int Commitment::ProcessMetadata(std::unique_ptr<TuskBlockMetadata> metadata) {
  std::unique_lock<std::mutex> lk(mutex_);
  // LOG(ERROR) << " @@@@ receive metadata round:" << metadata->round()
  //           << " from:" << metadata->sender_id();
  if (metadata->sender_id() == id_) {
    return 0;
  }
  for (const auto& signature : metadata->qc().certificates()) {
    bool valid = verifier_->VerifyMessage(metadata->hash(), signature);
    if (!valid) {
      LOG(ERROR) << "certificate is not valid";
      return -2;
    }
  }
  global_stats_->IncPropose();
  // LOG(ERROR) << "set metadata :" << metadata->round()
  //           << " from:" << metadata->source_id();
  latest_metadata_from_sender_[metadata->source_id()] =
      std::make_unique<TuskBlockMetadata>(*metadata);
  metadata_list_[metadata->round()][metadata->source_id()] =
      std::move(metadata);
  MayBroadCastBlock();
  return 0;
}

void Commitment::GenerateMetadata(const TuskRequest& new_block) {
  if (new_block.qc().certificates_size() < config_.GetMinDataReceiveNum()) {
    LOG(ERROR) << "certificate not valid";
    return;
  }

  auto metadata = std::make_unique<TuskBlockMetadata>();
  *metadata->mutable_qc() = new_block.qc();
  metadata->set_hash(new_block.hash());
  metadata->set_round(new_block.round());
  metadata->set_source_id(new_block.source_id());
  // LOG(ERROR) << "bc metadata round:" << metadata->round()<<"
  // id:"<<metadata->source_id();
  BroadCastMetadata(*metadata, TuskRequest::TYPE_METADATA);
  latest_metadata_from_sender_[metadata->source_id()] =
      std::make_unique<TuskBlockMetadata>(*metadata);
  metadata_list_[metadata->round()][metadata->source_id()] =
      std::move(metadata);
  MayBroadCastBlock();
  return;
}

bool Commitment::MayBroadCastBlock() {
  if (new_request_.empty()) {
    return false;
  }

  if (current_round_ == 0) {
    auto new_block = std::move(new_request_.front());
    new_request_.pop();
    new_block->set_round(current_round_);

    std::string data;
    new_block->SerializeToString(&data);
    new_block->set_hash(SignatureVerifier::CalculateHash(data));

    // LOG(ERROR) << "new block seq:" << new_block->round();
    BroadCastBlock(*new_block, TuskRequest::TYPE_NEW_BLOCK);
    candidate_[new_block->hash()] = std::move(new_block);
  } else {
    if (metadata_list_.find(current_round_ - 1) == metadata_list_.end()) {
      // LOG(ERROR) << "cetificate from round:" << (current_round_ - 1)
      //           << " not enough";
      return false;
    }

    if (metadata_list_[current_round_ - 1].size() <
        static_cast<size_t>(config_.GetMinDataReceiveNum())) {
      // LOG(ERROR) << "cetificate from round:" << (current_round_ - 1) << " not
      // enough:"<<metadata_list_[current_round_ - 1].size();
      return false;
    }

    if (metadata_list_[current_round_ - 1].find(id_) ==
        metadata_list_[current_round_ - 1].end()) {
      // LOG(ERROR)<<" wait self meta data, round:"<<current_round_-1<<"
      // id:"<<id_;
      return false;
    }

    auto new_block = std::move(new_request_.front());
    new_request_.pop();
    new_block->set_round(current_round_);
    std::set<int> meta_ids;
    for (const auto& preview_metadata : metadata_list_[current_round_ - 1]) {
      *new_block->add_strong_metadata() = *preview_metadata.second;
      meta_ids.insert(preview_metadata.first);
      // LOG(ERROR)<<"add strong round:"<<preview_metadata.second->round()<<"
      // id:"<<preview_metadata.second->source_id()<<"
      // "<<preview_metadata.first;
      if (meta_ids.size() ==
          static_cast<size_t>(config_.GetMinDataReceiveNum())) {
        break;
      }
    }
    // LOG(ERROR) << "round:"<<new_block->round()<<"
    // id:"<<new_block->source_id()<<" get metadata size:" <<
    // new_block->strong_metadata_size();
    for (const auto& meta : latest_metadata_from_sender_) {
      if (meta_ids.find(meta.first) != meta_ids.end()) {
        continue;
      }
      if (meta.second->round() > new_block->round()) {
        for (int j = current_round_; j >= 0; --j) {
          if (metadata_list_[j].find(meta.first) != metadata_list_[j].end()) {
            *new_block->add_weak_metadata() = *metadata_list_[j][meta.first];
            break;
          }
        }
      } else {
        assert(meta.second->round() <= new_block->round());
        *new_block->add_weak_metadata() = *meta.second;
      }
    }

    std::string data;
    new_block->SerializeToString(&data);
    new_block->set_hash(SignatureVerifier::CalculateHash(data));

    metadata_list_.erase(metadata_list_.find(current_round_ - 1));
    BroadCastBlock(*new_block, TuskRequest::TYPE_NEW_BLOCK);

    candidate_[new_block->hash()] = std::move(new_block);
  }
  current_round_++;
  if (current_round_ > 1) {
    if (current_round_ % 2 == 0) {
      AddLeaderRound(current_round_ - 2);
    } else if (current_round_ > 2) {
      AddLeaderRound(current_round_ - 3);
    }
  }

  return true;
}

void Commitment::ProcessCommit(std::unique_ptr<TuskRequest> leader_req,
                               int64_t previous_round) {
  std::queue<std::unique_ptr<TuskRequest>> q;
  q.push(std::move(leader_req));
  std::stack<std::unique_ptr<TuskRequest>> commit_list;
  std::set<std::pair<int64_t, int>> v;
  while (!q.empty()) {
    auto req = std::move(q.front());
    q.pop();
    for (const auto& meta : req->strong_metadata()) {
      if (v.find(std::make_pair(meta.round(), meta.source_id())) != v.end()) {
        continue;
      }
      auto strong_req = GetRequest(meta.round(), meta.source_id());
      if (strong_req == nullptr) {
        continue;
      }
      v.insert(std::make_pair(meta.round(), meta.source_id()));
      q.push(std::move(strong_req));
    }
    for (const auto& meta : req->weak_metadata()) {
      if (v.find(std::make_pair(meta.round(), meta.source_id())) != v.end()) {
        continue;
      }
      auto weak_req = GetRequest(meta.round(), meta.source_id());
      if (weak_req == nullptr) {
        continue;
      }
      v.insert(std::make_pair(meta.round(), meta.source_id()));
      q.push(std::move(weak_req));
    }
    commit_list.push(std::move(req));
  }

  while (!commit_list.empty()) {
    auto req = std::move(commit_list.top());
    commit_list.pop();
    if (message_manager_->IsCommitted(*req)) {
      continue;
    }
    int64_t round = req->round();
    int source_id = req->source_id();
    req->set_seq(current_seq_++);
    message_manager_->Commit(std::move(req));
    RemoveData(round, source_id);
  }
}

void Commitment::DAGConsensus() {
  int64_t previous_round = -2;
  while (!stop_) {
    int64_t round = GetLeaderRound();
    if (round == -1) {
      continue;
    }
    // LOG(ERROR) << "get consensus round:" << round
    //           << " previs:" << previous_round;
    int new_round = previous_round;
    for (int r = previous_round + 2; r <= round; r += 2) {
      int leader = GetLeader(r);
      auto req = GetRequest(r, leader);
      // LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<"
      // req:"<<(req==nullptr);
      if (req == nullptr) {
        continue;
      }
      int reference_num = GetReferenceNum(*req);
      if (reference_num < config_.GetMinDataReceiveNum()) {
        bool find = false;
        for (int j = r + 2; j <= round; j += 2) {
          int next_leader = GetLeader(j);
          auto next_req = GetRequest(j, next_leader);
          if (next_req != nullptr &&
              GetReferenceNum(*next_req) >= config_.GetMinDataReceiveNum()) {
            // LOG(ERROR) << "find next leader:" << next_req->round()
            //           << " source id:" << next_req->source_id();
            find = true;
            ProcessCommit(std::move(next_req), previous_round);
            r = j;
            break;
          }
        }
        if (!find) {
          // LOG(ERROR)<<"r:"<<r<<" leader:"<<leader<<" reference not enough";
          break;
        }
      } else {
        // LOG(ERROR) << "go to commit r:" << r << " previous:" <<
        // previous_round;
        ProcessCommit(std::move(req), previous_round);
      }
      new_round = r;
    }
    previous_round = new_round;
  }
}

}  // namespace tusk
}  // namespace resdb
