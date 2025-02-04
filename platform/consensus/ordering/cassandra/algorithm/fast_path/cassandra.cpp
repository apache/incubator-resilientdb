#include "platform/consensus/ordering/cassandra/algorithm/fast_path/cassandra.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_fp {

Cassandra::Cassandra(
    int id, int batch_size, int total_num, int f,
    std::function<int(int, google::protobuf::Message& msg, int)> single_call,
    std::function<int(int, google::protobuf::Message& msg)> broadcast_call,
    std::function<int(const Transaction& txn)> commit) {
  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  timeout_ms_ = 60000;
  local_txn_id_ = 1;
  local_proposal_id_ = 1;
  batch_size_ = batch_size;
  single_call_ = single_call;
  broadcast_call_ = broadcast_call;
  commit_ = commit;

  recv_num_ = 0;
  execute_num_ = 0;
  executed_ = 0;
  committed_num_ = 0;
  precommitted_num_ = 0;
  execute_id_ = 1;

  graph_ = std::make_unique<ProposalGraph>(f_, total_num_);
  proposal_manager_ =
      std::make_unique<ProposalManager>(id, total_num_, graph_.get());

  graph_->SetCommitCallBack(
      [&](const Proposal& proposal) { CommitProposal(proposal); });

  Reset();

  consensus_thread_ = std::thread(&Cassandra::AsyncConsensus, this);

  block_thread_ = std::thread(&Cassandra::BroadcastTxn, this);

  commit_thread_ = std::thread(&Cassandra::AsyncCommit, this);
}

Cassandra::~Cassandra() {
  is_stop_ = true;
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
}

bool Cassandra::IsStop() { return is_stop_; }

void Cassandra::Reset() {
  // received_num_.clear();
  // state_ = State::NewProposal;
}

void Cassandra::AsyncConsensus() {
  int height = 0;
  while (!is_stop_) {
    int next_height = SendTxn(height);
    if (next_height == -1) {
      // LOG(ERROR)<<"!!!!!! send txn fail sleep";
      // usleep(10000);
      proposal_manager_->WaitBlock();
      continue;
    }
    height = next_height;
    WaitVote(height);
  }
}

bool Cassandra::WaitVote(int height) {
  std::unique_lock<std::mutex> lk(mutex_);
  vote_cv_.wait_for(lk, std::chrono::microseconds(timeout_ms_ * 1000),
                    [&] { return can_vote_[height]; });
  if (!can_vote_[height]) {
    LOG(ERROR) << "wait vote time out"
               << " can vote:" << can_vote_[height] << " height:" << height;
  }
  return true;
}

void Cassandra::AsyncCommit() {
  while (!is_stop_) {
    std::unique_ptr<Proposal> p = execute_queue_.Pop(timeout_ms_ * 1000);
    if (p == nullptr) {
      LOG(ERROR) << "execu timeout";
      continue;
    }
    // LOG(ERROR) << "execute proposal from proposer:" <<
    // p->header().proposer_id()
    //           << " id:" << p->header().proposal_id()
    //           << " height:" << p->header().height()
    //           << " block size:" << p->block_size();

    for (const Block& block : p->block()) {
      std::unique_ptr<Block> data_block =
          proposal_manager_->GetBlock(block.hash(), p->header().proposer_id());
      // LOG(ERROR)<<"!!!!!!!!! commit proposal
      // from:"<<p->header().proposer_id()<<" txn
      // size:"<<data_block->data().transaction_size()<<"
      // height:"<<p->header().height();
      if (p->header().proposer_id() == id_) {
        execute_num_ += data_block->data().transaction_size();
        LOG(ERROR) << "recv num:" << recv_num_
                   << " execute num:" << execute_num_
                   << " block id:" << data_block->local_id() << " block delay:"
                   << (GetCurrentTime() - data_block->create_time());
      }

      for (Transaction& txn :
           *data_block->mutable_data()->mutable_transaction()) {
        txn.set_id(execute_id_++);
        commit_(txn);
      }
    }
  }
}

void Cassandra::CommitProposal(const Proposal& p) {
  LOG(ERROR) << "commit proposal from proposer:" << p.header().proposer_id()
             << " id:" << p.header().proposal_id()
             << " height:" << p.header().height()
             << " block size:" << p.block_size();
  if (p.block_size() == 0) {
    return;
  }
  // proposal_manager_->ClearProposal(p);
  committed_num_++;
  // LOG(ERROR) << "commit num:" << committed_num_
  //           << " commit delay:" << GetCurrentTime() - p.create_time();
  execute_queue_.Push(std::make_unique<Proposal>(p));
}

bool Cassandra::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txns_.Push(std::move(txn));
  recv_num_++;
  return true;
}

void Cassandra::BroadcastTxn() {
  std::vector<std::unique_ptr<Transaction>> txns;
  int num = 0;
  while (!IsStop()) {
    std::unique_ptr<Transaction> txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    txns.push_back(std::move(txn));
    if (txns.size() < batch_size_) {
      continue;
    }
    std::unique_ptr<Block> block = proposal_manager_->MakeBlock(txns);
    assert(block != nullptr);
    broadcast_call_(MessageType::NewBlocks, *block);
    proposal_manager_->AddLocalBlock(std::move(block));
    txns.clear();
  }
}

void Cassandra::ReceiveBlock(std::unique_ptr<Block> block) {
  // LOG(ERROR)<<"recv block:";
  proposal_manager_->AddBlock(std::move(block));
}

int Cassandra::SendTxn(int round) {
  std::unique_ptr<Proposal> proposal = nullptr;
  // LOG(ERROR)<<"send:"<<round;
  {
    round++;
    std::unique_lock<std::mutex> lk(mutex_);
    int current_round = proposal_manager_->CurrentRound();
    // LOG(ERROR)<<"current round:"<<current_round<<" send round:"<<round;
    assert(current_round < round);

    proposal = proposal_manager_->GenerateProposal(round, start_);
    if (proposal == nullptr) {
      LOG(ERROR) << "no transactions";
      if (start_ == false) {
        return -1;
      }
    }
    // start_ = true;
  }
  LOG(ERROR) << "bc proposal block size:" << proposal->block_size()
             << " round:" << round
             << " proposal height:" << proposal->header().height();
  broadcast_call_(MessageType::NewProposal, *proposal);
  assert(proposal->header().height() == round);
  return proposal->header().height();
}

bool Cassandra::ReceiveProposal(const Proposal& proposal) {
  {
    // LOG(ERROR)<<"recv proposal";
    std::unique_lock<std::mutex> lk(mutex_);
    const Proposal* pre_p =
        graph_->GetProposalInfo(proposal.header().prehash());
    if (pre_p == nullptr) {
      // LOG(ERROR) << "receive proposal from :" <<
      // proposal.header().proposer_id()
      //          << " id:" << proposal.header().proposal_id() << "no pre:";
    } else {
      // LOG(ERROR) << "receive proposal from :" <<
      // proposal.header().proposer_id()
      //          << " id:" << proposal.header().proposal_id()
      //          << "pre:" << pre_p->header().proposer_id()
      //          << " pre id:" << pre_p->header().proposal_id();
    }

    int current_time = GetCurrentTime();
    int wait_time = 0;
    if (received_time_.find(proposal.header().height()) ==
        received_time_.end()) {
      received_time_[proposal.header().height()] = current_time;
    } else {
      wait_time = current_time - received_time_[proposal.header().height()];
    }

    // LOG(ERROR) << "add proposal";
    if (!graph_->AddProposal(proposal)) {
      LOG(ERROR) << "add proposal fail";
      // TrySendRecoveery(proposal);
      return false;
    }

    std::vector<std::unique_ptr<Proposal>> future_g = graph_->GetNotFound(
        proposal.header().height() + 1, proposal.header().hash());
    if (future_g.size() > 0) {
      LOG(ERROR) << "get future size:" << future_g.size();
      for (auto& it : future_g) {
        if (!graph_->AddProposal(*it)) {
          LOG(ERROR) << "add future proposal fail";
          // TrySendRecoveery(proposal);
          continue;
        }

        received_num_[it->header().height()].insert(it->header().proposer_id());
        LOG(ERROR) << "received current height from not found:"
                   << graph_->GetCurrentHeight() << " num:"
                   << received_num_[graph_->GetCurrentHeight()].size()
                   << " from:" << it->header().proposer_id()
                   << " last vote:" << last_vote_;
      }
    }

    received_num_[proposal.header().height()].insert(
        proposal.header().proposer_id());
    // LOG(ERROR) << "received current height:" << graph_->GetCurrentHeight()
    //  << " proposal height:" << proposal.header().height()
    //  << " num:" << received_num_[graph_->GetCurrentHeight()].size()
    //  << " from:" << proposal.header().proposer_id()
    //           << " last vote:" << last_vote_
    //           << " wait time:"<<wait_time;
    bool check = true;
    if (received_num_[graph_->GetCurrentHeight()].size() == total_num_) {
      check = false;
      if (last_vote_ < graph_->GetCurrentHeight()) {
        graph_->CheckHistory(proposal);

        last_vote_ = graph_->GetCurrentHeight();
        can_vote_[graph_->GetCurrentHeight()] = true;
        vote_cv_.notify_all();
        // LOG(ERROR) << "can vote from full:";
      }
    }
    if (wait_time > 1000000 || !check) {
      if (received_num_[graph_->GetCurrentHeight()].size() == 2 * f_ + 1) {
        if (last_vote_ < graph_->GetCurrentHeight()) {
          graph_->CheckState(proposal);

          last_vote_ = graph_->GetCurrentHeight();
          can_vote_[graph_->GetCurrentHeight()] = true;
          vote_cv_.notify_all();
          // LOG(ERROR) << "can vote single:";
        }
      }
    }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}

}  // namespace cassandra_fp
}  // namespace cassandra
}  // namespace resdb
