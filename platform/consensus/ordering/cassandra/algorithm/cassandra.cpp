#include "platform/consensus/ordering/cassandra/algorithm/cassandra.h"

#include <glog/logging.h>

#include "common/crypto/signature_verifier.h"
#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {
namespace cassandra_recv {

Cassandra::Cassandra(int id, int f, int total_num, SignatureVerifier* verifier)
    : ProtocolBase(id, f, total_num), verifier_(verifier) {

  LOG(ERROR) << "get proposal graph";
  id_ = id;
  total_num_ = total_num;
  f_ = f;
  is_stop_ = false;
  timeout_ms_ = 60000;
  local_txn_id_ = 1;
  local_proposal_id_ = 1;
  batch_size_ = 10;

  recv_num_ = 0;
  execute_num_ = 0;
  executed_ = 0;
  committed_num_ = 0;
  precommitted_num_ = 0;
  execute_id_ = 1;

  graph_ = std::make_unique<ProposalGraph>(f_);
  proposal_manager_ = std::make_unique<ProposalManager>(id, graph_.get());

  graph_->SetCommitCallBack(
      [&](const Proposal& proposal) { CommitProposal(proposal); });

  Reset();

  consensus_thread_ = std::thread(&Cassandra::AsyncConsensus, this);

  block_thread_ = std::thread(&Cassandra::BroadcastTxn, this);

  commit_thread_ = std::thread(&Cassandra::AsyncCommit, this);

  global_stats_ = Stats::GetGlobalStats();

  prepare_thread_ = std::thread(&Cassandra::AsyncPrepare, this);
}

Cassandra::~Cassandra() {
  is_stop_ = true;
  if (consensus_thread_.joinable()) {
    consensus_thread_.join();
  }
  if (commit_thread_.joinable()) {
    commit_thread_.join();
  }
  if (prepare_thread_.joinable()) {
    prepare_thread_.join();
  }
}

void Cassandra::SetPrepareFunction(std::function<int(const Transaction&)> prepare){
  prepare_ = prepare;
}

bool Cassandra::IsStop() { return is_stop_; }

void Cassandra::Reset() {
  // received_num_.clear();
  // state_ = State::NewProposal;
}

void Cassandra::AsyncConsensus() {
  int height = 0;
  int64_t start_time = GetCurrentTime();
  while (!is_stop_) {
    int next_height = SendTxn(height);
    if (next_height == -1) {
      // usleep(10000);
      proposal_manager_->WaitBlock();
      continue;
    }
    int64_t end_time = GetCurrentTime();
    global_stats_->AddRoundLatency(end_time - start_time);
    start_time = end_time;
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
     //LOG(ERROR) << "execute proposal from proposer:" <<
     //        p->header().proposer_id()
     //          << " id:" << p->header().proposal_id()
     //          << " height:" << p->header().height()
     //          << " block size:" << p->block_size();

    int txn_num = 0;
    for (const Block& block : p->block()) {
      std::unique_lock<std::mutex> lk(mutex_);
      std::unique_ptr<Block> data_block =
          proposal_manager_->GetBlock(block.hash(), p->header().proposer_id());
       //LOG(ERROR)<<"!!!!!!!!! commit proposal from:"
      //<<p->header().proposer_id()<<" txn size:"
      //<<data_block->data().transaction_size()<<" height:"<<p->header().height();
      if (p->header().proposer_id() == id_) {
        execute_num_ += data_block->data().transaction_size();
        // LOG(ERROR) << "recv num:" << recv_num_
        //           << " execute num:" << execute_num_
        //           << " block id:" << data_block->local_id()
        //           << " block delay:" << (GetCurrentTime() -
        //           data_block->create_time());
      }

      for (Transaction& txn :
           *data_block->mutable_data()->mutable_transaction()) {
        txn.set_id(execute_id_++);
        txn_num++;
        commit_(txn);
      }
    }
    global_stats_->AddCommitTxn(txn_num);
  }
}

void Cassandra::CommitProposal(const Proposal& p) {
   //LOG(ERROR) << "commit proposal from proposer:" << p.header().proposer_id()
   //          << " id:" << p.header().proposal_id()
   //          << " height:" << p.header().height()
   //          << " block size:" << p.block_size();
  if (p.block_size() == 0) {
    return;
  }
  // proposal_manager_->ClearProposal(p);
  committed_num_++;
  int64_t commit_time = GetCurrentTime() - p.create_time();
  global_stats_->AddCommitLatency(commit_time);
  // LOG(ERROR) << "commit num:" << committed_num_
  //           << " commit delay:" << GetCurrentTime() - p.create_time();
  execute_queue_.Push(std::make_unique<Proposal>(p));
}

void Cassandra::AsyncPrepare() {
  while (!is_stop_) {
    std::unique_ptr<Proposal> p = prepare_queue_.Pop();
    if (p == nullptr) {
      //LOG(ERROR) << "execu timeout";
      continue;
    }
    //LOG(ERROR)<<"prepare block from:"<<p->header().proposer_id() <<" id:"<<p->header().proposal_id();
    int id = 0;
    for (const Block& block : p->block()) {
      std::unique_lock<std::mutex> lkx(mutex_);
      Block* data_block = proposal_manager_->GetBlockSnap(
          block.hash(), p->header().proposer_id());
      if (data_block == nullptr) {
        continue;
      }

      for (Transaction& txn :
           *data_block->mutable_data()->mutable_transaction()) {
        long long uid = (((long long)p->header().proposer_id() << 50) |
                         (long long)(p->header().proposal_id()) << 20 | id++);
        txn.set_uid(uid);
        prepare_(txn);
      }
    }

    // LOG(ERROR)<<" weak proposal:"<<p->weak_proposals().hash_size();
    for (const std::string& hash : p->weak_proposals().hash()) {
      const Proposal* pre_p = graph_->GetProposalInfo(hash);
      if (pre_p == nullptr) {
        continue;
      }
       //LOG(ERROR)<<"prepare weak block from:"<<pre_p->header().proposer_id()
       //<<" id:"<<pre_p->header().proposal_id();
      for (const Block& block : pre_p->block()) {
        std::unique_lock<std::mutex> lkx(mutex_);
        Block* data_block = proposal_manager_->GetBlockSnap(
            block.hash(), pre_p->header().proposer_id());
        if (data_block == nullptr) {
          continue;
        }

        for (Transaction& txn :
             *data_block->mutable_data()->mutable_transaction()) {
          long long uid = (((long long)pre_p->header().proposer_id() << 50) |
                           (long long)(pre_p->header().proposal_id()) << 20 | id++);
          txn.set_uid(uid);
          prepare_(txn);
        }
      }
    }
    //LOG(ERROR) << "prepare done";
  }
}

void Cassandra::PrepareProposal(const Proposal& p) {
  if (p.block_size() == 0) {
    return;
  }
  prepare_queue_.Push(std::make_unique<Proposal>(p));
}

bool Cassandra::ReceiveTransaction(std::unique_ptr<Transaction> txn) {
  // LOG(ERROR)<<"recv txn:";
  txn->set_create_time(GetCurrentTime());
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
    txn->set_queuing_time(GetCurrentTime()-txn->create_time());
    global_stats_->AddQueuingLatency(GetCurrentTime()-txn->create_time());
    //LOG(ERROR)<<"get txn, proxy id:"<<txn->proxy_id()<<" hash:"<<txn->hash();
    txns.push_back(std::move(txn));
    /*
    if (txns.size() < batch_size_) {
      continue;
    }
    */

    for(int i = 1; i < batch_size_; ++i){
      std::unique_ptr<Transaction> txn = txns_.Pop(0);
      if(txn == nullptr){
        break;
      }
      //LOG(ERROR)<<"get txn, proxy id:"<<txn->proxy_id()<<" hash:"<<txn->hash();
      txn->set_queuing_time(GetCurrentTime()-txn->create_time());
      global_stats_->AddQueuingLatency(GetCurrentTime()-txn->create_time());
      txns.push_back(std::move(txn));
    }

    std::unique_ptr<Block> block = proposal_manager_->MakeBlock(txns);
    assert(block != nullptr);
    //LOG(ERROR)<<" send block:"<<block->local_id();
    Broadcast(MessageType::NewBlocks, *block);
    proposal_manager_->AddLocalBlock(std::move(block));
    txns.clear();
  }
}

void Cassandra::ReceiveBlock(std::unique_ptr<Block> block) {
  // std::unique_lock<std::mutex> lk(g_mutex_);
  //LOG(ERROR)<<"recv block from:"<<block->sender_id()<<" block id:"<<block->local_id();
  BlockACK block_ack;
  block_ack.set_hash(block->hash());
  block_ack.set_sender_id(block->sender_id());
  block_ack.set_local_id(block->local_id());
  block_ack.set_responder(id_);

  // std::unique_lock<std::mutex> lk(mutex_);
  proposal_manager_->AddBlock(std::move(block));
  //LOG(ERROR)<<" send block to:"<<block_ack.sender_id();
  SendMessage(MessageType::CMD_BlockACK, block_ack, block_ack.sender_id());
}

void Cassandra::ReceiveBlockACK(std::unique_ptr<BlockACK> block) {
  //LOG(ERROR)<<"recv block ack:"<<block->local_id()<<" from:"<<block->responder();
  assert(block->sender_id() == id_);
  // std::unique_lock<std::mutex> lkx(g_mutex_);
  std::unique_lock<std::mutex> lk(block_mutex_);
  if (received_.find(block->local_id()) != received_.end()) {
    return;
  }
  block_ack_[block->local_id()].insert(block->responder());
  //LOG(ERROR)<<"recv block ack:"<<block->local_id()
  //<<" from:"<<block->responder()<< " num:"<<block_ack_[block->local_id()].size();
  if (block_ack_[block->local_id()].size() >= 2 * f_ + 1 &&
      block_ack_[block->local_id()].find(id_) !=
          block_ack_[block->local_id()].end()) {
    // std::unique_lock<std::mutex> lk(mutex_);
    proposal_manager_->BlockReady(block->hash(), block->local_id());
    received_.insert(block->local_id());
  }
}

int Cassandra::SendTxn(int round) {
  std::unique_ptr<Proposal> proposal = nullptr;
  // LOG(ERROR)<<"send:"<<round;
  {
    // std::unique_lock<std::mutex> lkx(g_mutex_);
    round++;
    std::unique_lock<std::mutex> lk(mutex_);
    int current_round = proposal_manager_->CurrentRound();
    //LOG(ERROR)<<"current round:"<<current_round<<" send round:"<<round;
    assert(current_round < round);

    proposal = proposal_manager_->GenerateProposal(round, start_);
    if (proposal == nullptr) {
      LOG(ERROR) << "no transactions";
      if (start_ == false) {
        return -1;
      }
    }

    if(!proposal->header().prehash().empty()){
      const Proposal* pre_p =
        graph_->GetProposalInfo(proposal->header().prehash());
        assert(pre_p != nullptr);
      PrepareProposal(*pre_p);
    }
  }

  //LOG(ERROR)<<"his size:"<<proposal->history_size();
  for (const auto& his : proposal->history()) {
    int sender = his.sender();
    int id = his.id();
    const std::string& hash = his.hash();
    int state = his.state();
    if (state != ProposalState::New) {
      continue;
    }
    const Proposal* p = graph_->GetProposalInfo(hash);
    assert(p);
    for (const auto& b : p->block()) {
      bool ret = proposal_manager_->ContainBlock(b.hash(), b.sender_id());
      //LOG(ERROR) << "sender:" << sender << " id:" << his.id()
      //           << " block id:" << b.local_id()
      //           << " block sender:" << b.sender_id() << " ret:" << ret
      //           << " state:" << state;
      assert(ret);
    }
  }
  proposal_manager_->AddLocalProposal(*proposal);

  //LOG(ERROR) << "====== bc proposal block size:" << proposal->block_size()
             //<< " round:" << round
             //<< " id:" << proposal->header().proposal_id()
             //<< " weak links:"<< proposal->weak_proposals().hash_size();

  Broadcast(MessageType::NewProposal, *proposal);

  assert(proposal->header().height() == round);
  //current_round_ = round;
  return proposal->header().height();
}

void Cassandra::SendBlock(const BlockQuery& block) {
  // std::unique_lock<std::mutex> lk(g_mutex_);
  // std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR)<<" query block from:"<<block.sender()<<" block id:"<<block.local_id();
  const std::string& hash = block.hash();
  const Block* block_resp = proposal_manager_->QueryBlock(hash);
  assert(block_resp != nullptr);
  //LOG(ERROR) << "send block :" << block.local_id() << " to :" << block.sender();
  SendMessage(MessageType::NewBlocks, *block_resp, block.sender());
}

void Cassandra::AskBlock(const Block& block) {
  BlockQuery query;
  query.set_hash(block.hash());
  query.set_proposer(block.sender_id());
  query.set_local_id(block.local_id());
  query.set_sender(id_);
  //LOG(ERROR) << "ask block from:" << block.sender_id();
  SendMessage(MessageType::CMD_BlockQuery, query, block.sender_id());
}

void Cassandra::AskProposal(const Proposal& proposal) {
  ProposalQuery query;
  query.set_hash(proposal.header().hash());
  query.set_proposer(proposal.header().proposer_id());
  query.set_id(proposal.header().proposal_id());
  query.set_sender(id_);
  //LOG(ERROR) << "ask proposal from:" << proposal.header().proposer_id();
  SendMessage(MessageType::CMD_ProposalQuery, query, proposal.header().proposer_id());
}

void Cassandra::SendProposal(const ProposalQuery& query) {
  std::unique_lock<std::mutex> lk(g_mutex_);
  // std::unique_lock<std::mutex> plk(mutex_);
  // std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR) << "!!!!! recv query proposal:" << query.id()
  //           << " from :" << query.sender();
  const std::string& hash = query.hash();
  std::unique_ptr<ProposalQueryResp> resp =
      proposal_manager_->QueryProposal(hash);
  assert(resp != nullptr);
  //LOG(ERROR) << "!!!!! send proposal:" << query.id()
  //           << " to :" << query.sender();
  SendMessage(MessageType::CMD_ProposalQueryResponse, *resp, query.sender());
}

void Cassandra::ReceiveProposalQueryResp(const ProposalQueryResp& resp) {
  // std::unique_lock<std::mutex> lkx(g_mutex_);
  std::unique_lock<std::mutex> lk(mutex_);
  //LOG(ERROR) << "!!!!! recv proposal query resp";
  proposal_manager_->VerifyProposal(resp);
}

bool Cassandra::ReceiveProposal(std::unique_ptr<Proposal> proposal) {
  // std::unique_lock<std::mutex> lk(g_mutex_);
  {
    // LOG(ERROR)<<"recv proposal";
    std::unique_lock<std::mutex> lk(mutex_);
    const Proposal* pre_p =
        graph_->GetProposalInfo(proposal->header().prehash());
    if (pre_p == nullptr) {
      //LOG(ERROR) << "receive proposal from :"
      //           << proposal->header().proposer_id()
      //           << " id:" << proposal->header().proposal_id() << "no pre:";
       if(!proposal->header().prehash().empty()) {
         if (proposal->header().height() > graph_->GetCurrentHeight()) {
           future_proposal_[proposal->header().height()].push_back(
               std::move(proposal));
           return true;
         }
       }
      assert(proposal->header().prehash().empty());
    } else {
      //LOG(ERROR) << "receive proposal from :"
      //           << proposal->header().proposer_id()
      //           << " id:" << proposal->header().proposal_id()
      //           << " pre:" << pre_p->header().proposer_id()
      //           << " pre id:" << pre_p->header().proposal_id();
    }

    /*
      if(proposal->header().height() >=200 && proposal->header().height() <205){
        if(!((proposal->header().proposer_id() <= 23 && id_ <=23)
        || (proposal->header().proposer_id() > 23 && id_ >23))) {
        //if(proposal->header().proposer_id() % 2 != id_%2){
          return true;
        }
      }
      */

    if (proposal->header().height() > graph_->GetCurrentHeight()) {
      future_proposal_[proposal->header().height()].push_back(
          std::move(proposal));
      return true;
    }
    AddProposal(*proposal);

    auto it = future_proposal_.find(graph_->GetCurrentHeight());
    if (it != future_proposal_.end()) {
      for (auto& p : it->second) {
        AddProposal(*p);
      }
      future_proposal_.erase(it);
    }
  }
  // LOG(ERROR)<<"receive proposal done";
  return true;
}

bool Cassandra::AddProposal(const Proposal& proposal) {
  int ret = proposal_manager_->VerifyProposal(proposal);
  if (ret != 0) {
    if (ret == 2) {
      LOG(ERROR) << "verify proposal fail";
      AskProposal(proposal);
    }
    return false;
  }

  for (const Block& block : proposal.block()) {
    bool ret = false;;
    for(int i = 0; i< 5; ++i){
      ret = proposal_manager_->ContainBlock(block);
      if (ret == false) {
        LOG(ERROR) << "======== block from:" << block.sender_id()
          << " block id:" << block.local_id() << " not exist";
         usleep(1000);
        //AskBlock(block);
        continue;
      }
      else {
        break;
      }
    }
    assert(ret);
  }

  {
    std::unique_lock<std::mutex> lk(g_mutex_);
    // LOG(ERROR) << "add proposal";
    int v_ret = graph_->AddProposal(proposal);
    if (v_ret != 0) {
      LOG(ERROR) << "add proposal fail, ret:" << v_ret;
      if (v_ret == 2) {
        // miss history
        // AskProposal(proposal);
      }
      // TrySendRecoveery(proposal);
      return false;
    }

    if (proposal.header().proposer_id() == id_) {
      proposal_manager_->RemoveLocalProposal(proposal.header().hash());
    }
  }

  received_num_[proposal.header().height()].insert(
      proposal.header().proposer_id());
  //LOG(ERROR) << "received current height:" << graph_->GetCurrentHeight()
  //           << " proposal height:" << proposal.header().height()
  //           << " num:" << received_num_[graph_->GetCurrentHeight()].size()
  //           << " from:" << proposal.header().proposer_id()
  //           << " last vote:" << last_vote_;
  if (received_num_[graph_->GetCurrentHeight()].size() == total_num_) {
    if (last_vote_ < graph_->GetCurrentHeight()) {
      last_vote_ = graph_->GetCurrentHeight();
      can_vote_[graph_->GetCurrentHeight()] = true;
      vote_cv_.notify_all();
      //LOG(ERROR) << "can vote:";
    }
  }
  // LOG(ERROR)<<"recv done";

  std::vector<std::unique_ptr<Proposal>> future_g = graph_->GetNotFound(
      proposal.header().height() + 1, proposal.header().hash());
  if (future_g.size() > 0) {
    // LOG(ERROR) << "get future size:" << future_g.size();
    for (auto& it : future_g) {
      if (!graph_->AddProposal(*it)) {
        LOG(ERROR) << "add future proposal fail";
        // TrySendRecoveery(proposal);
        continue;
      }

      received_num_[it->header().height()].insert(it->header().proposer_id());
      //LOG(ERROR) << "received current height:" << graph_->GetCurrentHeight()
      //           << " num:" << received_num_[graph_->GetCurrentHeight()].size()
      //           << " from:" << it->header().proposer_id()
      //           << " last vote:" << last_vote_;
    }
  }
  return true;
}

}  // namespace cassandra_recv
}  // namespace cassandra
}  // namespace resdb
