#include "platform/consensus/ordering/tusk/protocol/tusk.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace tusk {

Tusk::Tusk(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  LOG(ERROR) << "id:" << id << " f:" << f << " total:" << total_num;
  limit_count_ = 2 * f + 1;
  batch_size_ = 1;
  // next_seq_ = 1;
  local_txn_id_ = 1;
  execute_id_ = 1;
  start_ = 0;
  dag_id_ = 0;
  txn_pending_num_ = 0;
  CreateManager(dag_id_);
  // totoal_proposer_num_ = total_num_;

  send_thread_ = std::thread(&Tusk::AsyncSend, this);
  commit_thread_ = std::thread(&Tusk::AsyncCommit, this);
  execute_thread_ = std::thread(&Tusk::AsyncExecute, this);
  future_thread_ = std::thread(&Tusk::RunFutureBlock, this);
  future_cert_thread_ = std::thread(&Tusk::RunFutureCert, this);
  // block_thread_ = std::thread(&Tusk::AsyncBlock, this);
}

Tusk::~Tusk() {
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
}

int Tusk::GetLeader(int64_t r) { return r / 2 % total_num_ + 1; }

void Tusk::StopDone() { SwitchDAG(); }

void Tusk::CreateManager(int dag) {
  if (proposal_manager_ == nullptr) {
    proposal_manager_ =
        std::make_unique<ProposalManager>(id_, limit_count_, dag, total_num_);
  }
}

bool Tusk::WaitReady() {
  while (!IsStop()) {
    std::unique_lock<std::mutex> lk(mutex_);
    // LOG(ERROR)<<" wait leader";
    vote_cv_.wait_for(lk, std::chrono::microseconds(1000000),
                      [&] { return proposal_manager_->ReadyWithLeader(); });
    if (!proposal_manager_->ReadyWithLeader()) {
      LOG(ERROR) << " wait leader";
      return false;
    }
    return true;
  }
  return true;
}

void Tusk::NotifyInput() {
  txn_pending_num_ = 0;
  send_cv_.notify_all();
}

void Tusk::WaitBatchReady() {
  while (!IsStop()) {
    {
      std::unique_lock<std::mutex> lk(send_mutex_);
      send_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return txn_pending_num_ < batch_size_; });
      // LOG(ERROR)<<" pending txn round:"<<proposal_manager_->CurrentRound()<<"
      // num:"<<txn_pending_num_;
      if (txn_pending_num_ < batch_size_) {
        txn_pending_num_++;
        return;
      }
    }
  }
}

bool Tusk::WaitForNext(bool is_cross) {
  while (!IsStop()) {
    /// LOG(ERROR)<<" wait batch";
    WaitBatchReady();

    int current_round = proposal_manager_->CurrentRound();
    /// LOG(ERROR)<<" wait for next round:"<<current_round;
    /*
    if (current_round == 1 || (current_round %2)) {
      return;
    }
    */

    if (!is_cross) {
      if (!WaitReady()) {
        LOG(ERROR) << " wait leader fail";
        return false;
      }
    }

    std::unique_lock<std::mutex> lk(cross_mutex_);
    while (!future_cross_txn_[dag_id_].empty() &&
           future_cross_txn_[dag_id_].begin()->first < current_round) {
      int round = future_cross_txn_[dag_id_].begin()->first;
      int proposer = future_cross_txn_[dag_id_].begin()->second;
      future_cross_txn_[dag_id_].erase(future_cross_txn_[dag_id_].begin());
      cross_txn_[dag_id_].insert(std::make_pair(round, proposer));
    }

    // LOG(ERROR)<<" corss txn size:"<<cross_txn_[dag_id_].size()<<"
    // txn:"<<txn_num_[current_round];
    if (cross_txn_[dag_id_].empty()) {
      // LOG(ERROR)<<" no corss";
      return true;
    }
    // LOG(ERROR)<<" has corss";
    return false;
  }
  return true;
}

void Tusk::AddCrossTxn(int dag_id, int round, int proposer) {
  std::unique_lock<std::mutex> lk(cross_mutex_);
  // LOG(ERROR)<<" add cross txn round:"<<round<<" proposer:"<<proposer;
  future_cross_txn_[dag_id].insert(std::make_pair(round, proposer));
}

void Tusk::RelaseCrossTxn(int dag_id, int round, int proposer) {
  std::unique_lock<std::mutex> lk(cross_mutex_);
  auto it = cross_txn_[dag_id].find(std::make_pair(round, proposer));
  if (it != cross_txn_[dag_id].end()) {
    // LOG(ERROR)<<" release round:"<<round<<" proposer:"<<proposer;
    cross_txn_[dag_id].erase(it);
  }
  cross_cv_.notify_all();
}

ProposalManager* Tusk::GetManager(int dag) { return proposal_manager_.get(); }

void Tusk::SwitchDAG() {
  std::unique_lock<std::mutex> lk(switch_mutex_);

  for (auto& txn : pending_txn_) {
    txns_.Push(std::move(txn));
  }
  pending_txn_.clear();

  auto pm = GetManager(dag_id_);
  assert(pm != nullptr);
  std::vector<std::unique_ptr<Transaction>> ret = pm->GetTxn();
  for (auto& txn : ret) {
    txns_.Push(std::move(txn));
  }

  dag_id_++;
  LOG(ERROR) << " switch to dag:" << dag_id_;
  received_stop_.clear();
  previous_round_ = -2;
  pm->Reset(dag_id_);
  stop_commit_ = false;
  last_id_ = 0;
}

void Tusk::SendTransactions(
    const std::vector<std::unique_ptr<Transaction>>& txns) {
  auto proposal_manager = GetManager(dag_id_);

  std::unique_lock<std::mutex> lk(switch_mutex_);
  auto proposal = proposal_manager->GenerateProposal(txns);
  Broadcast(MessageType::NewBlock, *proposal);
  proposal_manager->AddLocalBlock(std::move(proposal));

  int round = proposal_manager->CurrentRound() - 1;
  // if(last_id_ != dag_id_) {
  LOG(ERROR) << "bc txn:" << txns.size() << " round:" << round
             << " dag id:" << dag_id_;
  // last_id_ = dag_id_;
  //}
  if (round > 1) {
    if (round % 2 == 0) {
      CommitRound(round - 2);
    }
  }
  // LOG(ERROR)<<" send txn";
}

void Tusk::AsyncSend() {
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      if (start_) {
        LOG(ERROR) << " no data";
      }
      continue;
    }

    auto proposal_manager = GetManager(dag_id_);

    while (!IsStop()) {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return proposal_manager->Ready(); });

      std::unique_lock<std::mutex> plk(switch_mutex_);
      if (proposal_manager->Ready()) {
        start_ = 1;
        break;
      }
    }

    std::unique_lock<std::mutex> slk(send_mutex_);
    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto txn = txns_.Pop(0);
      if (txn == nullptr) {
        break;
      }
      txns.push_back(std::move(txn));
    }

    std::unique_lock<std::mutex> lk(switch_mutex_);
    auto proposal = proposal_manager->GenerateProposal(txns);
    Broadcast(MessageType::NewBlock, *proposal);
    proposal_manager->AddLocalBlock(std::move(proposal));

    int round = proposal_manager->CurrentRound() - 1;
    // LOG(ERROR) << "bc txn:" << txns.size() << " round:" << round<<" dag
    // id:"<<dag_id_;; if(last_id_ != dag_id_) {
    LOG(ERROR) << "bc txn:" << txns.size() << " round:" << round
               << " dag id:" << dag_id_;
    // last_id_ = dag_id_;
    //}
    if (round > 1) {
      if (round % 2 == 0) {
        CommitRound(round - 2);
        CheckCrossRound(round);
      }
    }
    NotifyInput();
  }
}

void Tusk::AsyncCommit() {
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      // LOG(ERROR) << "execu timeout";
      continue;
    }

    std::unique_lock<std::mutex> lk(switch_mutex_);
    int& previous_round = previous_round_;
    int round = *round_or;
    // LOG(ERROR) << "commit round:" << round<<" previous
    // round:"<<previous_round<<" dag:"<<dag_id_;
    assert(round >= previous_round);
    auto proposal_manager = GetManager(dag_id_);

    int new_round = previous_round;
    for (int r = previous_round + 2; r <= round; r += 2) {
      int leader = GetLeader(r);
      auto req = proposal_manager->GetRequest(r, leader);
      // LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<"
      // req:"<<(req==nullptr);
      if (req == nullptr) {
        continue;
      }
      int reference_num = proposal_manager->GetReferenceNum(*req);
      // LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<"
      // ref:"<<reference_num;
      if (reference_num < limit_count_) {
        continue;
      } else {
        // LOG(ERROR) << "go to commit r:" << r << " previous:" <<
        // previous_round;
        for (int j = new_round + 2; j <= r; j += 2) {
          int leader = GetLeader(j);
          CommitProposal(j, leader);
        }
      }
      new_round = r;
    }
    previous_round = new_round;
    // LOG(ERROR) << "commit round done:" << round<<" previous
    // round:"<<previous_round<<" dag:"<<dag_id_;
  }
}

void Tusk::AsyncExecute() {
  int commit_round = 0;
  while (!IsStop()) {
    std::unique_ptr<const Proposal*> t_proposal = execute_queue_.Pop();
    if (t_proposal == nullptr) {
      // LOG(ERROR) << "execu timeout";
      continue;
    }
    const Proposal* proposal = *t_proposal;
    assert(proposal != nullptr);
    std::unique_lock<std::mutex> lk(switch_mutex_);
    if (stop_commit_) {
      // LOG(ERROR)<<" stop commit from dag:"<<dag_id_<<"
      // header:"<<proposal->header().dag_id();
      assert(proposal->header().dag_id() <= dag_id_);
      continue;
    }
    // LOG(ERROR)<<" commit proposal dag:"<<dag_id_<<"
    // from:"<<proposal->header().proposer_id();
    if (proposal->header().dag_id() != dag_id_) {
      // LOG(ERROR)<<" commit proposal dag:"<<proposal->header().dag_id()<<"
      // fail current:"<<dag_id_; assert(1==0);
      continue;
    }
    std::map<int, int> max_round, max_process;
    commit_round++;
    int proposal_round = proposal->header().round();
    max_round[proposal->header().proposer_id()] = proposal_round;

    for (auto& link : proposal->header().strong_cert().cert()) {
      int link_round = link.round();
      int link_proposer = link.proposer();
      // LOG(ERROR)<<" commit proposal
      // proposer:"<<proposal->header().proposer_id()<<"
      // round:"<<proposal->header().round()<<" contain
      // proposer:"<<link_proposer<<" round:"<<link_round;
      max_round[link_proposer] = link_round;
    }
    //   std::map<int,int> max_process = GetMaxCommitRound(proposal.get());
    //

    std::map<int, std::vector<const Proposal*>> ps;
    std::queue<const Proposal*> q;
    q.push(proposal);
    std::set<std::pair<int, int>> in_q;

    auto proposal_manager = GetManager(dag_id_);
    while (!q.empty()) {
      const Proposal* p = q.front();
      q.pop();
      // LOG(ERROR)<<" get proposer:"<<p->header().proposer_id()<<"
      // dag:"<<p->header().dag_id()<<" follow dag:"<<dag_id_<<"
      // timeout:"<<p->time_out()<<" round:"<<p->header().round();
      assert(p->header().dag_id() == dag_id_);

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        if (in_q.find(std::make_pair(link_proposer, link_round)) !=
            in_q.end()) {
          continue;
        }
        bool valid = true;
        if (link.shard_flag() > 0) {
          for (int i = 0; i < 64; ++i) {
            if ((1ll << i) & link.shard_flag()) {
              if (max_round[i + 1] != proposal_round - 1) {
                // LOG(ERROR)<<" coheck round:"<<max_round[i+1]<<"
                // proposer:"<<i+1<<" proposal round:"<<proposal_round<<" link
                // round:"<<link_round;
                valid = false;
                break;
              }
            }
          }
        }
        // LOG(ERROR)<<" commit fetch:"<<link_round<<"
        // proposer:"<<link_proposer; LOG(ERROR)<<" commit fetch:"<<link_round<<"
        // proposer:"<<link_proposer<<" valid:"<<valid<<"
        // flag:"<<link.shard_flag();
        if (!valid) {
          if (max_process.find(link_proposer) == max_process.end() ||
              max_process[link_proposer] > link_round) {
            // LOG(ERROR)<<" commit fetch:"<<link_round<<"
            // proposer:"<<link_proposer<<" valid:"<<valid<<"
            // flag:"<<link.shard_flag()<<"
            // maxpross:"<<max_process[link_proposer];
            max_process[link_proposer] = link_round;
          }
        }
        auto next_p = proposal_manager->GetRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          // LOG(ERROR)<<"no data round:"<<link_round<<" link
          // proposer:"<<link_proposer;
          continue;
        }
        // LOG(ERROR)<<" get request round:"<<link_round<<"
        // proposer:"<<link_proposer<<" header:"<<next_p->header().round()<<"
        // proposer:"<<next_p->header().proposer_id();
        in_q.insert(std::make_pair(link_proposer, link_round));
        q.push(next_p);
      }

      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        if (in_q.find(std::make_pair(link_proposer, link_round)) !=
            in_q.end()) {
          continue;
        }
        bool valid = true;
        if (link.shard_flag() > 0) {
          for (int i = 0; i < 64; ++i) {
            if ((1ll << i) & link.shard_flag()) {
              if (max_round[i + 1] != proposal_round - 1) {
                // LOG(ERROR)<<" coheck round:"<<max_round[i+1]<<"
                // proposer:"<<i+1<<" proposal round:"<<proposal_round<<" link
                // round:"<<link_round;
                valid = false;
                break;
              }
            }
          }
        }
        // LOG(ERROR)<<" commit fetch:"<<link_round<<"
        // proposer:"<<link_proposer<<" valid:"<<valid<<"
        // flag:"<<link.shard_flag();
        if (!valid) {
          if (max_process.find(link_proposer) == max_process.end() ||
              max_process[link_proposer] > link_round) {
            // LOG(ERROR)<<" commit fetch:"<<link_round<<"
            // proposer:"<<link_proposer<<" valid:"<<valid<<"
            // flag:"<<link.shard_flag()<<"
            // maxpross:"<<max_process[link_proposer];
            max_process[link_proposer] = link_round;
          }
        }
        // LOG(ERROR)<<" commit fetch:"<<link_round<<"
        // proposer:"<<link_proposer;
        auto next_p = proposal_manager->GetRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          // LOG(ERROR)<<"no data round:"<<link_round<<" link
          // proposer:"<<link_proposer;
          continue;
        }
        // LOG(ERROR)<<" get request round:"<<link_round<<"
        // proposer:"<<link_proposer<<" header:"<<next_p->header().round()<<"
        // proposer:"<<next_p->header().proposer_id();
        in_q.insert(std::make_pair(link_proposer, link_round));
        q.push(next_p);
      }
      ps[p->header().round()].push_back(p);
    }

    // LOG(ERROR)<<" lock done";
    bool roll_back = false;
    for (auto& it : ps) {
      for (auto& p : it.second) {
        int proposer = p->header().proposer_id();
        int round = p->header().round();
        bool empty = false;
        int max_pro = 0;
        // LOG(ERROR)<<" commit proposer:"<<p->header().proposer_id()<<"
        // round:"<<p->header().round()<<" max process:"<<max_pro<<" max
        // round"<<max_round[proposer]<<" proposal
        // round:"<<proposal_round<<"shard flag:"<<p->header().shard_flag();
        if (max_process.find(proposer) != max_process.end()) {
          max_pro = max_process[proposer];
          if (round >= max_pro) {
            if (proposal_round - round > 20) {
              // LOG(ERROR)<<" commit proposer:"<<p->header().proposer_id()<<"
              // round:"<<p->header().round()<<" max process:"<<max_pro<<"
              // skip"<<" proposal round:"<<proposal_round;
              empty = true;
              // assert(1==0);
            } else {
              // LOG(ERROR)<<" commit proposer:"<<p->header().proposer_id()<<"
              // round:"<<p->header().round()<<" max process:"<<max_pro<<"
              // skip"<<" proposal round:"<<proposal_round;
              continue;
            }
          }
        }
        // LOG(ERROR)<<" commit proposer:"<<p->header().proposer_id()<<"
        // round:"<<p->header().round()<<" max process:"<<max_pro<<"
        // empty:"<<empty<<" shard flag:"<<p->header().shard_flag()<<" proposal
        // round:"<<proposal_round;
        assert(p->header().dag_id() == dag_id_);
        if (static_cast<int>(received_stop_.size()) >= 2 * f_ + 1) {
          roll_back = true;
        }
        if (p->stop() && !roll_back) {
          // LOG(ERROR)<<" receive stop from:"<<p->header().proposer_id()<<"
          // size:"<<received_stop_.size();
          received_stop_.insert(p->header().proposer_id());
        }

        RelaseCrossTxn(p->header().dag_id(), p->header().round(),
                       p->header().proposer_id());

        auto data_p = proposal_manager->FetchRequest(round, proposer);
        assert(data_p != nullptr);

        int num = data_p->transactions_size();
        /*
          LOG(ERROR) << "=============== commit proposal round :"
            << p->header().round()
            << " proposer:" << p->header().proposer_id()
            << " transaction size:" << p->transactions_size()
            << " commit time:"
            << (GetCurrentTime() - p->header().create_time())
            << " create time:" << p->header().create_time()
            <<" execute id:"<<execute_id_
            <<" is stop:"<<p->stop()
            <<" current dag:"<<dag_id_
            <<" num:"<<num
            <<" roll back:"<<roll_back
            <<" dag:"<<p->header().dag_id();
            */

        for (auto& tx : *data_p->mutable_transactions()) {
          if (p->header().proposer_id() == id_) {
            tx.set_create_time(p->header().create_time());
          }
          tx.set_group_id(commit_round);
          tx.set_flag(0);
          if (p->stop() && (--num == 0)) {
            int proposer = p->header().proposer_id();
            int dag = p->header().dag_id();
            tx.set_flag((dag << 16) | proposer);
          }
          if (empty) {
            tx.set_skip(true);
            // LOG(ERROR)<<" clear data";
          }
          // int txn_id = tx.id();
          if (roll_back) {
            if (p->header().proposer_id() == id_) {
              // LOG(ERROR)<<" add back txn:"<<" dag:"<<dag_id_<<"
              // txn:"<<txn_id<<" from proposer:"<<p->header().proposer_id();
              AddBackTxn(tx);
            }
          } else {
            tx.set_id(execute_id_++);
            // LOG(ERROR)<<" dag :"<<p->header().dag_id()<<" commit
            // txn:"<<execute_id_<<" from proposer:"<<p->header().proposer_id();
            Commit(tx);
          }
        }
      }
    }
    if (static_cast<int>(received_stop_.size()) >= 2 * f_ + 1) {
      // SwitchDAG();
      stop_commit_ = true;
    }
    // LOG(ERROR)<<" commit done stop:"<<stop_commit_;
  }
}

bool Tusk::HasCheck(int round, int proposer) {
  return has_check_.find(std::make_pair(round, proposer)) != has_check_.end();
}

void Tusk::SetCheck(int round, int proposer) {
  has_check_.insert(std::make_pair(round, proposer));
}

void Tusk::AddIfCrossTxn(const Proposal& p) {
  if (p.header().shard_flag() == 0) {
    return;
  }
  // LOG(ERROR)<<" check cross proposal round:"<<p.header().round()<<"
  // proposer:"<<p.header().proposer_id()<<" shard
  // flag:"<<p.header().shard_flag();
  if (p.header().shard_flag() & (1ll << (id_ - 1))) {
    AddCrossTxn(dag_id_, p.header().round(), p.header().proposer_id());
  }
}

void Tusk::CheckCrossRound(int round) {
  // LOG(ERROR)<<" corss check round:"<<round;
  auto proposal_manager = GetManager(dag_id_);
  assert(proposal_manager != nullptr);

  int leader = GetLeader(round);
  // LOG(ERROR)<<" get round:"<<round<<" leader:"<<leader;
  if (leader == id_) {
    return;
  }
  auto req = proposal_manager->GetRequest(round, leader);
  if (req == nullptr) {
    return;
  }
  assert(req != nullptr);
  CheckCross(*req);
}

void Tusk::CheckCross(const Proposal& proposal) {
  if (proposal.header().dag_id() != dag_id_) {
    return;
  }

  std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
  std::queue<const Proposal*> q;
  q.push(&proposal);

  auto proposal_manager = GetManager(dag_id_);
  while (!q.empty()) {
    const Proposal* p = q.front();
    q.pop();
    assert(p->header().dag_id() == dag_id_);

    int round = p->header().round();
    int proposer = p->header().proposer_id();
    // LOG(ERROR)<<" check round :"<<round<<" proposer:"<<proposer;
    if (HasCheck(round, proposer)) {
      continue;
    }

    SetCheck(round, proposer);

    AddIfCrossTxn(*p);

    for (auto& link : p->header().strong_cert().cert()) {
      int link_round = link.round();
      int link_proposer = link.proposer();
      if (HasCheck(link_round, link_proposer)) {
        continue;
      }
      auto next_p = proposal_manager->GetRequest(link_round, link_proposer);
      if (next_p == nullptr) {
        continue;
      }
      q.push(next_p);
    }

    for (auto& link : p->header().weak_cert().cert()) {
      int link_round = link.round();
      int link_proposer = link.proposer();
      if (HasCheck(link_round, link_proposer)) {
        continue;
      }
      auto next_p = proposal_manager->GetRequest(link_round, link_proposer);
      if (next_p == nullptr) {
        continue;
      }
      q.push(next_p);
    }
  }
  // LOG(ERROR)<<" check done:";
}

void Tusk::CommitProposal(int round, int proposer) {
  int64_t now_time = GetCurrentTime();
  LOG(ERROR) << "commit round:" << round << " proposer:" << proposer
             << " dag:" << dag_id_ << " time:" << now_time - last_time_;
  last_time_ = now_time;
  auto proposal_manager = GetManager(dag_id_);
  const Proposal* p = proposal_manager->GetRequest(round, proposer);
  if (p == nullptr) {
    return;
  }
  assert(p != nullptr);
  std::unique_ptr<const Proposal*> tmp = std::make_unique<const Proposal*>(p);
  execute_queue_.Push(std::move(tmp));
}

void Tusk::CommitRound(int round) {
  commit_queue_.Push(std::make_unique<int>(round));
}

void Tusk::AddBackTxn(const Transaction& tx) {
  pending_txn_.push_back(std::make_unique<Transaction>(tx));
}

bool Tusk::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn from:"<<txn->proxy_id();
  txn->set_id(local_txn_id_++);

#ifndef Async
  txns_.Push(std::move(txn));
  {
    std::unique_lock<std::mutex> lk(switch_mutex_);
    txn_num_[proposal_manager_->CurrentRound()]++;
    txn_cv_.notify_all();
  }
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
#else
  vec_txns_.push_back(std::move(txn));
  if (static_cast<int>(vec_txns_.size()) == batch_size_) {
    SendTransactions(vec_txns_);
    vec_txns_.clear();
  }
#endif
  return true;
}

void Tusk::SetVerifyFunc(std::function<bool(const Transaction& txn)> func) {
  verify_func_ = func;
}

void Tusk::RunFutureBlock() {
  while (!IsStop()) {
    auto Check = [&]() {
      if (future_block_map_.empty()) {
        return false;
      }
      auto it = future_block_map_.begin();
      if (it->first > dag_id_) {
        return false;
      }
      return true;
    };

    std::vector<std::unique_ptr<Proposal>> list;
    {
      std::unique_lock<std::mutex> lk(block_mutex_);
      block_cv_.wait_for(lk, std::chrono::microseconds(1000),
                         [&] { return Check(); });

      if (!Check()) {
        continue;
      }

      auto it = future_block_map_.begin();
      if (it->first > dag_id_) {
        break;
      }
      list = std::move(it->second);

      future_block_map_.erase(it);
    }

    for (auto& p : list) {
      // LOG(ERROR)<<" run future proposal dag:"<<p->header().dag_id()<<"
      // now:"<<dag_id_
      //  <<" proposer:"<<p->header().proposer_id() <<"
      //  round:"<<p->header().round();
      ReceiveBlock(std::move(p));
    }
  }
}

bool Tusk::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  // LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
  //           << " round:" << proposal->header().round()
  //           <<" dag:"<<dag_id_<<" proposal dag:"<<proposal->header().dag_id()
  //           <<" shard flag:"<<proposal->header().shard_flag();
  int dag = proposal->header().dag_id();
  // std::unique_lock<std::mutex> lk(switch_mutex_);
  // int round = proposal->header().round();
  // int proposer = proposal->header().proposer_id();

  if (proposal->header().dag_id() != dag_id_) {
    if (dag_id_ < proposal->header().dag_id()) {
      // LOG(ERROR)<<" header dag not match:"<<proposal->header().dag_id()<<"
      // now:"<<dag_id_;
      // std::unique_lock<std::mutex> lk(p_mutex_);
      // pending_.Push(std::move(proposal));

      std::unique_lock<std::mutex> lk(block_mutex_);
      future_block_map_[dag].push_back(std::move(proposal));
      block_cv_.notify_all();
      return false;
    }
  }
  auto proposal_manager = GetManager(dag);
  // assert(proposal_manager->VerifyHash(*proposal));

  Metadata metadata;
  metadata.set_sender(id_);
  metadata.set_hash(proposal->hash());
  metadata.set_round(proposal->header().round());
  metadata.set_proposer(proposal->header().proposer_id());
  metadata.set_dag_id(proposal->header().dag_id());
  metadata.set_shard_flag(proposal->header().shard_flag());

  std::string data = proposal->hash();
  auto hash_signature_or = verifier_->SignMessage(data);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *metadata.mutable_sign() = *hash_signature_or;

  {
    if (dag_id_ == proposal->header().dag_id()) {
      proposal_manager->AddBlock(std::move(proposal));
    }
  }
  SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}

void Tusk::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  int dag_id = metadata->dag_id();
  uint64_t shard_flag = metadata->shard_flag();
  // assert(dag_id == dag_id_);
  std::unique_lock<std::mutex> slk(switch_mutex_);
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[dag_id][hash][sender] = std::move(metadata);
  received_flag_[dag_id][hash] |= 1ll << (sender - 1);
  auto proposal_manager = GetManager(dag_id);
  // LOG(ERROR) << "recv block ack from:" << sender
  //           << " num:" << received_num_[dag_id][hash].size()
  //           <<" flag:"<<shard_flag
  //           <<" receive flag:"<<received_flag_[dag_id][hash];
  if (static_cast<int>(received_num_[dag_id][hash].size()) >= limit_count_) {
    Certificate cert;
    for (auto& it : received_num_[dag_id][hash]) {
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    cert.set_dag_id(dag_id);
    cert.set_shard_flag(shard_flag);
    const Proposal* p = proposal_manager->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    *cert.mutable_strong_cert() = p->header().strong_cert();
    // LOG(ERROR)<<"send cert, round:"<<p->header().round();
    Broadcast(MessageType::Cert, cert);
    received_num_[dag_id][hash].clear();
  }
}

bool Tusk::VerifyCert(const Certificate& cert) {
  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for (auto& metadata : cert.metadata()) {
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if (!valid) {
      return false;
    }
    hash_num[metadata.hash()]++;
    if (hash_num[metadata.hash()] >= 2 * f_ + 1) {
      vote_num = true;
    }
  }
  return vote_num;
}

void Tusk::RunFutureCert() {
  while (!IsStop()) {
    auto Check = [&]() {
      if (future_cert_map_.empty()) {
        return false;
      }
      auto it = future_cert_map_.begin();
      if (it->first > dag_id_) {
        return false;
      }
      return true;
    };

    std::vector<std::unique_ptr<Certificate>> list;
    {
      std::unique_lock<std::mutex> lk(cert_mutex_);
      cert_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return Check(); });

      if (!Check()) {
        continue;
      }

      auto it = future_cert_map_.begin();
      if (it->first > dag_id_) {
        break;
      }
      list = std::move(it->second);

      future_cert_map_.erase(it);
    }

    for (auto& cert : list) {
      // LOG(ERROR)<<"future cert:"<<cert->proposer()<<" dag:"<<dag_id_<<" cert
      // dag:"
      //  <<cert->dag_id()<<" round:"<<cert->round()<<"
      //  proposer:"<<cert->proposer();
      ReceiveBlockCert(std::move(cert));
    }
  }
}

void Tusk::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int dag_id = cert->dag_id();

  if (!VerifyCert(*cert)) {
    assert(1 == 0);
    return;
  }

  {
    std::unique_lock<std::mutex> slk(switch_mutex_);
    // LOG(ERROR)<<"recv cert:"<<cert->proposer()<<" dag:"<<dag_id_<<" cert
    // dag:"
    //<<cert->dag_id()<<" round:"<<cert->round()<<"
    //proposer:"<<cert->proposer();
    if (cert->dag_id() > dag_id_) {
      std::unique_lock<std::mutex> lk(cert_mutex_);
      future_cert_map_[dag_id].push_back(std::move(cert));
      cert_cv_.notify_all();
      return;
    } else if (cert->dag_id() < dag_id_) {
      return;
    }

    auto proposal_manager = GetManager(dag_id);
    assert(proposal_manager != nullptr);
    proposal_manager->AddCert(std::move(cert));
  }
  {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
}

}  // namespace tusk
}  // namespace resdb
