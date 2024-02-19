#include "platform/consensus/ordering/tusk/algorithm/tusk.h"

#include <glog/logging.h>

#include "common/utils/utils.h"

namespace resdb {
namespace tusk {

Tusk::Tusk(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {
  limit_count_ = 2 * f + 1;
  batch_size_ = 2;
  proposal_manager_ = std::make_unique<ProposalManager>(id, limit_count_);

  execute_id_ = 1;
  start_ = 0;
  queue_size_ = 0;

  send_thread_ = std::thread(&Tusk::AsyncSend, this);
  commit_thread_ = std::thread(&Tusk::AsyncCommit, this);
  execute_thread_ = std::thread(&Tusk::AsyncExecute, this);

  global_stats_ = Stats::GetGlobalStats();

  LOG(ERROR) << "id:" << id << " f:" << f << " total:" << total_num;
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

void Tusk::AsyncSend() {

  uint64_t last_time = 0;
  while (!IsStop()) {
    auto txn = txns_.Pop();
    if (txn == nullptr) {
      if(start_){
        //LOG(ERROR)<<"not enough txn";
      }
      continue;
    }

    queue_size_--;
    //LOG(ERROR)<<"txn before waiting time:"<<GetCurrentTime() - txn->create_time()<<" queue size:"<<queue_size_;
    while (!IsStop()) {
      std::unique_lock<std::mutex> lk(mutex_);
      vote_cv_.wait_for(lk, std::chrono::microseconds(1000),
                        [&] { return proposal_manager_->Ready(); });

      if (proposal_manager_->Ready()) {
        start_ = 1;
        break;
      }
    }
    //LOG(ERROR)<<"txn waiting time:"<<GetCurrentTime() - txn->create_time();
    txn->set_queuing_time(GetCurrentTime() - txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
    global_stats_->AddRoundLatency(GetCurrentTime() - last_time);
    last_time = GetCurrentTime();

    std::vector<std::unique_ptr<Transaction>> txns;
    txns.push_back(std::move(txn));
    for (int i = 1; i < batch_size_; ++i) {
      auto txn = txns_.Pop(0);
      if (txn == nullptr) {
        break;
      }
      txn->set_queuing_time(GetCurrentTime() - txn->create_time());
      queue_size_--;
      global_stats_->AddQueuingLatency(GetCurrentTime() - txn->create_time());
      txns.push_back(std::move(txn));
    }

    global_stats_->ConsumeTransactions(queue_size_);

    auto proposal = proposal_manager_->GenerateProposal(txns);
    Broadcast(MessageType::NewBlock, *proposal);
    proposal_manager_->AddLocalBlock(std::move(proposal));

    int round = proposal_manager_->CurrentRound() - 1;
    //LOG(ERROR) << "bc txn:" << txns.size() << " round:" << round;
    if (round > 1) {
      if (round % 2 == 0) {
        CommitRound(round - 2);
      }
    }
  }
}

void Tusk::AsyncCommit() {
  int previous_round = -2;
  while (!IsStop()) {
    std::unique_ptr<int> round_or = commit_queue_.Pop();
    if (round_or == nullptr) {
      continue;
    }

    int round = *round_or;
    int64_t start_time = GetCurrentTime();
    //LOG(ERROR) << "commit round:" << round;

    int new_round = previous_round;
    for (int r = previous_round + 2; r <= round; r += 2) {
      int leader = GetLeader(r);
      auto req = proposal_manager_->GetRequest(r, leader);
      // req:"<<(req==nullptr);
      if (req == nullptr) {
        continue;
      }
      // LOG(ERROR)<<" get leader:"<<leader<<" round:"<<r<<" delay:"<<(GetCurrentTime() - req->header().create_time());
      int reference_num = proposal_manager_->GetReferenceNum(*req);
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
    int64_t end_time = GetCurrentTime();
    //global_stats_->AddCommitRuntime(end_time-commit_time);
    previous_round = new_round;
  }
}

void Tusk::AsyncExecute() {
int64_t last_commit_time = 0;
  while (!IsStop()) {
    std::unique_ptr<Proposal> proposal = execute_queue_.Pop();
    if (proposal == nullptr) {
      continue;
    }

    int commit_round = proposal->header().round();
    int64_t commit_time = GetCurrentTime();
    int64_t waiting_time = commit_time - proposal->queuing_time();
    global_stats_->AddCommitQueuingLatency(waiting_time);
    global_stats_->AddCommitInterval(commit_time - last_commit_time);
    last_commit_time = commit_time;
    std::map<int, std::vector<std::unique_ptr<Proposal>>> ps;
    std::queue<std::unique_ptr<Proposal>> q;
    q.push(std::move(proposal));

    int64_t start_time = GetCurrentTime();
    while (!q.empty()) {
      std::unique_ptr<Proposal> p = std::move(q.front());
      q.pop();

      for (auto& link : p->header().strong_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          continue;
        }
        q.push(std::move(next_p));
      }


      for (auto& link : p->header().weak_cert().cert()) {
        int link_round = link.round();
        int link_proposer = link.proposer();
        auto next_p =
            proposal_manager_->FetchRequest(link_round, link_proposer);
        if (next_p == nullptr) {
          // LOG(ERROR)<<"no data round:"<<link_round<<" link
          // proposer:"<<link_proposer;
          continue;
        }
        q.push(std::move(next_p));
      }
      ps[p->header().round()].push_back(std::move(p));
    }

    int num = 0;
    for (auto& it : ps) {
      for (auto& p : it.second) {
        //LOG(ERROR) << "=============== commit proposal round :"
        //           << p->header().round()
        //           << " header round:"<<p->header().round()
        //           << " commit round:"<<commit_round
        //           << " proposer:" << p->header().proposer_id()
        //           << " transaction size:" << p->transactions_size()
        //           << " commit time:"
        //           << (GetCurrentTime() - p->header().create_time())
        //           << " create time:" << p->header().create_time()
        //           <<" execute id:"<<execute_id_ 
        //           <<" round delay:"<<(commit_round - p->header().round());

        global_stats_->AddCommitLatency(commit_time - p->header().create_time() - waiting_time);
        global_stats_->AddCommitRoundLatency(commit_round - p->header().round());
        for (auto& tx : *p->mutable_transactions()) {
          tx.set_id(execute_id_++);
          num++;
          //LOG(ERROR)<<" commit txn create time:"<<tx.create_time();
          Commit(tx);
        }
      }
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddCommitRuntime(end_time-commit_time);
    global_stats_->AddCommitTxn(num);
  }
}

void Tusk::CommitProposal(int round, int proposer) {
  //LOG(ERROR) << "commit round:" << round << " proposer:" << proposer;
  std::unique_ptr<Proposal> p =
      proposal_manager_->FetchRequest(round, proposer);
  if(p==nullptr){
    LOG(ERROR)<<"commit round:"<<round<<" proposer:"<<proposer;
    return;
  }
  assert(p !=nullptr);
  p->set_queuing_time(GetCurrentTime());
  execute_queue_.Push(std::move(p));
}

void Tusk::CommitRound(int round) {
  commit_queue_.Push(std::make_unique<int>(round));
}

bool Tusk::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  //  LOG(ERROR)<<"recv txn";
  txn->set_create_time(GetCurrentTime());
  txns_.Push(std::move(txn));
  queue_size_++;
  if (start_ == 0) {
    std::unique_lock<std::mutex> lk(mutex_);
    vote_cv_.notify_all();
  }
  return true;
}

bool Tusk::ReceiveBlock(std::unique_ptr<Proposal> proposal) {
  //LOG(ERROR) << "recv block from " << proposal->header().proposer_id()
  //           << " round:" << proposal->header().round();
  assert(proposal_manager_->VerifyHash(*proposal));

  Metadata metadata;
  metadata.set_sender(id_);
  metadata.set_hash(proposal->hash());
  metadata.set_round(proposal->header().round());
  metadata.set_proposer(proposal->header().proposer_id());

  std::string data = proposal->hash();
  auto hash_signature_or = verifier_->SignMessage(data);
  if (!hash_signature_or.ok()) {
    LOG(ERROR) << "Sign message fail";
    return false;
  }
  *metadata.mutable_sign()=*hash_signature_or;

  {
    proposal_manager_->AddBlock(std::move(proposal));
  }
  // LOG(ERROR)<<"send ack:"<<metadata.proposer();
  SendMessage(MessageType::BlockACK, metadata, metadata.proposer());
  return true;
}

void Tusk::ReceiveBlockACK(std::unique_ptr<Metadata> metadata) {
  std::string hash = metadata->hash();
  int round = metadata->round();
  int sender = metadata->sender();
  std::unique_lock<std::mutex> lk(txn_mutex_);
  received_num_[hash][sender] = std::move(metadata);
  //LOG(ERROR) << "recv block ack from:" << sender
  //           << " num:" << received_num_[hash].size();
  if (received_num_[hash].size() == limit_count_) {
    Certificate cert;
    for (auto& it : received_num_[hash]) {
      *cert.add_metadata() = *it.second;
    }
    cert.set_hash(hash);
    cert.set_round(round);
    cert.set_proposer(id_);
    const Proposal* p = proposal_manager_->GetLocalBlock(hash);
    assert(p != nullptr);
    assert(p->header().proposer_id() == id_);
    assert(p->header().round() == round);
    //global_stats_->AddCommitLatency(GetCurrentTime() - p->header().create_time());
    *cert.mutable_strong_cert() = p->header().strong_cert();
    //LOG(ERROR)<<"send cert, round:"<<p->header().round();
    Broadcast(MessageType::Cert, cert);
  }
}

bool Tusk::VerifyCert(const Certificate & cert){
  std::map<std::string, int> hash_num;
  bool vote_num = false;
  for(auto& metadata: cert.metadata()){
    bool valid = verifier_->VerifyMessage(metadata.hash(), metadata.sign());
    if(!valid){
      return false;
    }
    hash_num[metadata.hash()]++;
    if(hash_num[metadata.hash()]>=2*f_+1){
      vote_num = true;
    }
  }
  return vote_num;
}


void Tusk::ReceiveBlockCert(std::unique_ptr<Certificate> cert) {
  int64_t start_time = GetCurrentTime();
  if(!VerifyCert(*cert)){
    assert(1==0);
    return;
  }
  int64_t end_time = GetCurrentTime();
  global_stats_->AddVerifyLatency(end_time-start_time); 


  //LOG(ERROR)<<"recv cert:"<<cert->proposer() <<" round:"<< cert->round();
  proposal_manager_->AddCert(std::move(cert));
  std::unique_lock<std::mutex> lk(mutex_);
  vote_cv_.notify_all();
}

}  // namespace tusk
}  // namespace resdb
