#include "platform/consensus/ordering/rcc/protocol/rcc.h"

#include <glog/logging.h>

namespace resdb {
namespace rcc {

RCC::RCC(int id, int f, int total_num,
         protocol::ProtocolBase::SingleCallFuncType single_call,
         protocol::ProtocolBase::BroadcastCallFuncType broadcast_call,
         protocol::ProtocolBase::CommitFuncType commit)
    : ProtocolBase(id, f, total_num, single_call, broadcast_call, commit) {
  proposal_manager_ = std::make_unique<ProposalManager>(id);
  next_seq_ = 1;
  local_txn_id_ = 1;
  execute_id_ = 1;
  totoal_proposer_num_ = total_num_;
  batch_size_ = 30;

  send_thread_ = std::thread(&RCC::AsyncSend, this);
  commit_thread_ = std::thread(&RCC::AsyncCommit, this);
}

RCC::~RCC() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
}

void RCC::AsyncCommit() {
  while (!IsStop()) {
    std::unique_ptr<Proposal> msg = execute_queue_.Pop();
    if (msg == nullptr) {
      // LOG(ERROR) << "execu timeout";
      continue;
    }
    int seq = msg->header().seq();
    int proposer = msg->header().proposer_id();
    seq_set_[seq][proposer] = std::move(msg);

    while (!IsStop()) {
      if (seq_set_[next_seq_].size() == totoal_proposer_num_) {
        for (auto& it : seq_set_[next_seq_]) {
          // LOG(ERROR)<<"next seq:"<<next_seq_<<" transaction
          // size:"<<it.second->transactions_size();
          for (auto& txn : *it.second->mutable_transactions()) {
            txn.set_id(execute_id_++);
            commit_(txn);
          }
        }
        next_seq_++;
      } else {
        break;
      }
    }
  }
}

void RCC::CommitProposal(std::unique_ptr<Proposal> p) {
  execute_queue_.Push(std::move(p));
}

bool RCC::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  {
    std::unique_lock<std::mutex> lk(txn_mutex_);
    txn->set_id(local_txn_id_++);
    txns_.Push(std::move(txn));
  }
  return true;
}

void RCC::AsyncSend() {
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      continue;
    }

    std::vector<std::unique_ptr<Transaction> > txns;
    for (int i = 0; i < batch_size_; i++) {
      txns.push_back(std::move(txn));
      txn = txns_.Pop(0);
      if (txn == nullptr) {
        break;
      }
    }

    std::unique_ptr<Proposal> proposal =
        proposal_manager_->GenerateProposal(txns);

    // LOG(ERROR) << "send proposal id:" << id_ << " seq:" <<
    // proposal->header().seq();
    broadcast_call_(MessageType::ConsensusMsg, *proposal);
  }
  return;
}

bool RCC::ReceiveProposal(const Proposal& proposal) {
  int proposer = proposal.header().proposer_id();
  int seq = proposal.header().seq();
  int sender = proposal.header().sender();
  int status = proposal.header().status();

  std::unique_ptr<Proposal> data;
  std::string hash = proposal.header().hash();

  if (status == ProposalType::NewMsg) {
    data = std::make_unique<Proposal>(proposal);
  }

  bool changed = false;
  {
    std::unique_lock<std::mutex> lk(mutex_[proposer]);

    if (status != ProposalType::NewMsg) {
      if (is_commit_[proposer].find(seq) != is_commit_[proposer].end()) {
        return 0;
      }
    }

    // LOG(ERROR) << "received :" << seq
    //           << " num:" << received_num_[proposer][seq][status].size()
    //           << " from:" << sender
    //           << " proposer:"<< proposer
    //           << " status:" << proposal.header().status()
    //           << " 2f+1:"<< 2*f_+1;

    auto it = collector_[proposer].find(hash);
    if (it == collector_[proposer].end()) {
      collector_[proposer].insert(
          std::make_pair(hash, std::make_unique<TransactionCollector>(seq)));
      it = collector_[proposer].find(hash);
    } else {
      assert(it->second->GetSeq() == seq);
    }

    it->second->AddRequest(
        status == ProposalType::NewMsg ? std::move(data) : nullptr, sender,
        status,
        [&](const google::protobuf::Message&, int received_count,
            std::atomic<TransactionStatue>* s) {
          if (status == ProposalType::NewMsg || received_count >= 2 * f_ + 1) {
            switch (status) {
              case ProposalType::NewMsg:
                if (*s == TransactionStatue::None) {
                  *s = TransactionStatue::READY_PREPARE, changed = true;
                }
                break;
              case ProposalType::Prepared:
                if (*s == TransactionStatue::READY_PREPARE) {
                  *s = TransactionStatue::READY_COMMIT;
                  changed = true;
                }
                break;
              case ProposalType::Commit:
                if (*s == TransactionStatue::READY_COMMIT) {
                  *s = TransactionStatue::READY_EXECUTE;
                  changed = true;
                }
                break;
            }
          }
          // LOG(ERROR)<<"status:"<<status<<" count:"<<received_count<<"
          // changed:"<<changed;
        });
  }

  if (changed) {
    Proposal new_proposal;
    *new_proposal.mutable_header() = proposal.header();
    new_proposal.mutable_header()->set_sender(id_);
    UpgradeState(&new_proposal);
    if (new_proposal.header().status() == ProposalType::Ready_execute) {
      std::unique_lock<std::mutex> lk(mutex_[proposer]);
      // LOG(ERROR)<<"obtaind ata proposer:"<<proposer<<" seq:"<<seq;
      auto it = collector_[proposer].find(hash);
      assert(it != collector_[proposer].end());

      std::unique_ptr<google::protobuf::Message> data = it->second->GetData();

      std::unique_ptr<Proposal> raw_p(dynamic_cast<Proposal*>(data.release()));

      collector_[proposer].erase(it);

      assert(raw_p != nullptr);
      is_commit_[proposer].insert(seq);

      // LOG(ERROR)<<"commit type:"<<new_proposal.header().status()<<"
      // transaction size:"<<raw_p->transactions_size();
      CommitProposal(std::move(raw_p));
    } else {
      // LOG(ERROR)<<"bc type:"<<new_proposal.header().status();
      Broadcast(MessageType::ConsensusMsg, new_proposal);
    }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}

void RCC::UpgradeState(Proposal* proposal) {
  switch (proposal->header().status()) {
    case ProposalType::NewMsg:
      proposal->mutable_header()->set_status(ProposalType::Prepared);
      return;
    case ProposalType::Prepared:
      proposal->mutable_header()->set_status(ProposalType::Commit);
      return;
    case ProposalType::Commit:
      proposal->mutable_header()->set_status(ProposalType::Ready_execute);
      return;
  }
}

}  // namespace rcc
}  // namespace resdb
