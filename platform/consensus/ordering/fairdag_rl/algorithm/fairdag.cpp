#include "platform/consensus/ordering/fairdag_rl/algorithm/fairdag.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace fairdag_rl {

FairDAG::FairDAG(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), replica_num_(total_num) {
  LOG(ERROR) << "id:" << id << " f:" << f << " total:" << replica_num_;

  tusk_ = std::make_unique<Tusk>(
      id, f, total_num, verifier,
      [&](int type, const google::protobuf::Message& msg, int node) {
        return SendMessage(type, msg, node);
      },
      [&](int type, const google::protobuf::Message& msg) {
        return Broadcast(type, msg);
      },
      [&](std::vector<std::unique_ptr<Transaction>>& txns) {
        return CommitTxns(txns);
      });
  graph_ = std::make_unique<Graph>();
  local_time_ = 0;
  execute_id_ = 1;
  global_stats_ = Stats::GetGlobalStats();
}

FairDAG::~FairDAG() {}

bool FairDAG::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  std::unique_lock<std::mutex> lk(mutex_);
  // txn->set_timestamp(local_time_++);
  // txn->set_proposer(id_);
  // LOG(ERROR)<<"recv txn from:"<<txn->proxy_id()<<" proxy user
  // seq:"<<txn->user_seq();
  return tusk_->ReceiveTransaction(std::move(txn));
}

bool FairDAG::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  return tusk_->ReceiveBlock(std::move(proposal));
}

void FairDAG::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  tusk_->ReceiveBlockACK(std::move(metadata));
}

void FairDAG::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  tusk_->ReceiveBlockCert(std::move(cert));
}

bool FairDAG::IsCommitted(const Transaction& txn) {
  std::pair<int, int64_t> key = std::make_pair(txn.proxy_id(), txn.user_seq());
  return committed_.find(key) != committed_.end();
}

void FairDAG::SetCommitted(const Transaction& txn) {
  std::pair<int, int64_t> key = std::make_pair(txn.proxy_id(), txn.user_seq());
  committed_.insert(key);
}

void FairDAG::AddTxnData(std::unique_ptr<Transaction> txn) {
  std::pair<int, int64_t> key =
      std::make_pair(txn->proxy_id(), txn->user_seq());
  if (data_.find(key) == data_.end()) {
    data_[key] = std::move(txn);
  }
}

void FairDAG::CommitTxns(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  std::vector<int> commit_txns;
  std::map<int, Transaction*> hash2txn;
  std::vector<std::pair<int, int>> pendings;

  std::map<int, std::vector<int>> proposals;

  for (int i = 0; i < txns.size(); ++i) {
    if (IsCommitted(*txns[i])) {
      continue;
    }
    int proposer = txns[i]->proposer();
    std::pair<int, int64_t> key =
        std::make_pair(txns[i]->proxy_id(), txns[i]->user_seq());
    if (commit_proposers_idx_.find(key) == commit_proposers_idx_.end()) {
      proposals_key_[idx_] = key;
      commit_proposers_idx_[key] = idx_++;
      proposals_data_[key] = txns[i]->hash();
    }
    commit_proposers_[key].insert(proposer);

    int id = commit_proposers_idx_[key];
    proposals[proposer].push_back(id);
  }

  std::set<std::pair<int, int>> nedges;
  for (auto it : proposals) {
    int proposer = it.first;
    for (int i = 0; i < it.second.size(); ++i) {
      for (int j = i + 1; j < it.second.size(); ++j) {
        int id1 = it.second[i];
        int id2 = it.second[j];
        if (id1 == id2) {
          continue;
        }

        auto e = std::make_pair(id1, id2);
        // LOG(ERROR)<<" new edges:("<<id1<<","<<id2<<")"<<"
        // proposer:"<<proposer<<" size:"<<it.second.size();
        edge_counts_[e].insert(proposer);
        nedges.insert(e);
      }
    }
  }

  for (auto it : proposals) {
    int proposer = it.first;
    if (pending_proposals_[proposer].empty()) {
      continue;
    }
    for (int i = 0; i < it.second.size(); ++i) {
      // LOG(ERROR)<<" old txn proposer:"<<proposer<<"
      // size:"<<pending_proposals_[proposer].size();
      for (auto pit : pending_proposals_[proposer]) {
        int id1 = pit;
        int id2 = it.second[i];
        auto e = std::make_pair(id1, id2);
        // LOG(ERROR)<<" old edges:("<<id1<<","<<id2<<")"<<"
        // proposer:"<<proposer;
        edge_counts_[e].insert(proposer);
        nedges.insert(e);
      }
    }
  }

  for (auto it : proposals) {
    int proposer = it.first;
    for (int i = 0; i < it.second.size(); ++i) {
      int id = it.second[i];
      pending_proposals_[proposer].insert(id);
    }
    // LOG(ERROR)<<" add to pending proposer:"<<proposer<<"
    // size:"<<pending_proposals_[proposer].size();
  }

  std::set<int> ids;
  for (auto e : nedges) {
    if (edge_counts_[e].size() < f_ + 1) {
      continue;
    }
    int id1 = e.first;
    int id2 = e.second;
    ids.insert(id1);
    ids.insert(id2);
    auto pe = std::make_pair(id2, id1);
    // LOG(ERROR)<<" edges:("<<id1<<","<<id2<<")"<<"
    // cound:"<<edge_counts_[e].size()<<" "<<edge_counts_[pe].size();
    if (edge_counts_[e].size() > edge_counts_[pe].size()) {
      graph_->AddTxn(id1, id2);
    } else {
      graph_->AddTxn(id2, id1);
    }
  }
  for (int id : ids) {
    commit_txns.push_back(id);
  }

  // LOG(ERROR)<<" add:"<<graph_->Size();
  if (commit_txns.empty()) {
    return;
  }
  std::vector<int> orders = graph_->GetOrder(commit_txns);

  int last = orders.size() - 1;
  for (; last >= 0; --last) {
    int id = orders[last];
    auto key = proposals_key_[id];
    if (commit_proposers_[key].size() >= 2 * f_ + 1) {
      break;
    }
  }
  // LOG(ERROR)<<" orders size:"<<orders.size()<<" last:"<<last;

  if (last < 0) {
    return;
  }

  for (int i = 0; i < last; ++i) {
    int id = orders[i];
    assert(proposals_key_.find(id) != proposals_key_.end());
    auto key = proposals_key_[id];
    assert(proposals_data_.find(key) != proposals_data_.end());
    std::string hash = proposals_data_[key];

    // std::pair<int,int64_t> key = std::make_pair(res[i]->proxy_id(),
    // res[i]->user_seq()); int id = commit_proposers_idx_[key]; assert(res[i] !=
    // nullptr);
    std::unique_ptr<Transaction> data = tusk_->FetchTxn(hash);
    if (data == nullptr) {
      LOG(ERROR) << "no txn";
    }
    assert(data != nullptr);
    data->set_id(execute_id_++);
    SetCommitted(*data);
    Commit(*data);

    graph_->RemoveTxn(id);

    for (auto proposer : commit_proposers_[key]) {
      if (pending_proposals_[proposer].find(id) !=
          pending_proposals_[proposer].end()) {
        // LOG(ERROR)<<" remove id:"<<id<<" from proposer:"<<proposer;
        pending_proposals_[proposer].erase(
            pending_proposals_[proposer].find(id));
      }
    }
  }
  return;
}

}  // namespace fairdag_rl
}  // namespace resdb
