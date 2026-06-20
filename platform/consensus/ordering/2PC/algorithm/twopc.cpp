#include "platform/consensus/ordering/2PC/algorithm/twopc.h"

#include <glog/logging.h>

namespace resdb {
namespace twopc {

TwoPC::TwoPC(int id, int f, int total_num) : ProtocolBase(id, f, total_num) {}

int TwoPC::StartTransaction(const std::string& transaction_id,
                            const Request& request) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_transactions_[transaction_id] = request;
  }
  Request prepare_msg;
  prepare_msg.set_data(request.data());
  prepare_msg.set_hash(transaction_id);
  prepare_msg.set_seq(request.seq());
  prepare_msg.set_uid(request.uid());
  prepare_msg.set_proxy_id(request.proxy_id());
  return Broadcast(MsgType::PREPARE, prepare_msg);
}

int TwoPC::ProcessVote(const std::string& transaction_id, int node_id,
                       bool vote) {
  bool should_decide = false;
  bool all_yes = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    transaction_votes_[transaction_id].push_back(vote);
    int expected = total_num_ - 1;
    if ((int)transaction_votes_[transaction_id].size() >= expected) {
      all_yes = true;
      for (bool v : transaction_votes_[transaction_id]) {
        if (!v) {
          all_yes = false;
          break;
        }
      }
      transaction_votes_.erase(transaction_id);
      should_decide = true;
    }
  }
  if (should_decide) {
    Request decision_msg;
    decision_msg.set_hash(transaction_id);
    if (all_yes) {
      Broadcast(MsgType::COMMIT, decision_msg);
    } else {
      Broadcast(MsgType::ABORT, decision_msg);
    }
  }
  return 0;
}

int TwoPC::ProcessPrepare(const std::string& transaction_id,
                          const Request& request) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_transactions_[transaction_id] = request;
  }
  bool valid = ValidateTransaction(request);
  return SendVote(transaction_id, valid);
}

bool TwoPC::ValidateTransaction(const Request& request) { return true; }

int TwoPC::SendVote(const std::string& transaction_id, bool vote) {
  Request vote_msg;
  vote_msg.set_hash(transaction_id);
  vote_msg.set_seq(vote ? 1 : 0);
  return SendMessage(MsgType::VOTE, vote_msg, coordinator_id_);
}

int TwoPC::ProcessDecision(const std::string& transaction_id, bool commit) {
  if (commit) {
    Request commit_request;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = pending_transactions_.find(transaction_id);
      if (it == pending_transactions_.end()) {
        return 0;
      }
      commit_request = it->second;
      pending_transactions_.erase(it);
    }
    Commit(commit_request);
  } else {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_transactions_.erase(transaction_id);
  }
  return 0;
}

}  // namespace twopc
}  // namespace resdb