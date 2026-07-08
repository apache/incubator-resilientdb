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

#include "platform/consensus/ordering/cassandra/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include "common/utils/utils.h"

namespace resdb {
namespace cassandra {

namespace {

struct GroupMoveState {
  bool started = false;
};

std::set<int> cur_fail;
std::mutex g_mutex_;
GroupMoveState g_group_move_state;

bool EnableGroupMove() {
  return true;
  const char* flag = std::getenv("RESDB_ENABLE_GROUP_MOVE");
  return flag != nullptr && std::string(flag) == "1";
}

std::vector<int> BuildMoveOrder(int total_replicas) {
  const int f = total_replicas / 3;
  const int group_size = 3;
  std::vector<int> move_order;
  move_order.reserve(total_replicas);
  if (f <= 0) {
    return move_order;
  }

  for (int moved_group = 0; moved_group < f; ++moved_group) {
    int group_index = (1 + moved_group * 2) % f;
    int start_id = group_index * group_size + 1;
    int end_id = std::min(start_id + group_size - 1, total_replicas);
    for (int node_id = start_id; node_id <= end_id; ++node_id) {
      move_order.push_back(node_id);
    }
  }
  return move_order;
}

void StartGroupMoveThread(int total_replicas) {
  const int f = total_replicas / 3;
  const std::vector<int> move_order = BuildMoveOrder(total_replicas);
  std::thread([total_replicas, move_order, f]() {
    int target = 0;
    int target1 = f;
    int f_time = 2;
    bool use_f1 = false;
    if(use_f1) {
      f_time = 5;
      target = f;
      target1 = 0;
    }
    else  {
      f_time = 2;
      target = f+2; 
      if(target1>0){
        //target = f+2;
        f_time = 2;
      }
    }

    if(target1>0) {
      sleep(10);

      {
        std::unique_lock<std::mutex> lk(g_mutex_);
        for(int i = 10; i <=total_replicas && cur_fail.size()<target1; i++) {
          LOG(ERROR)<<"move node1:"<<i;
          cur_fail.insert(i);
        }
      }
    }

    sleep(10);

    while(cur_fail.size()<target) {
	    {
		    std::unique_lock<std::mutex> lk(g_mutex_);
		    for(int i = 10; i <=total_replicas && cur_fail.size()<target; i++) {
			    if(((i-10)/3)%2==1){
			//	    continue;
			    }
			    LOG(ERROR)<<"move node2:"<<i;

			    int old = cur_fail.size();
			    cur_fail.insert(i);
			    if(cur_fail.size() != old) {
				    //break;
			    }
		    }
	    }
	    //usleep(200000);
    }

    sleep(10);
    //sleep(f_time);

    if(target1>0) {
      std::unique_lock<std::mutex> lk(g_mutex_);
      while (cur_fail.size()>target1) {
        LOG(ERROR) << "recover node:" << *cur_fail.begin();
        cur_fail.erase(cur_fail.begin());
      }
    }
    sleep(10);
    {
      while (!cur_fail.empty()) {
	      {
      std::unique_lock<std::mutex> lk(g_mutex_);
        LOG(ERROR) << "recover node:" << *cur_fail.begin();
        cur_fail.erase(cur_fail.begin());
	      }
//	usleep(200000);
      }
    }
  }).detach();
}

bool IsCrossPartitionBlocked(int self_id, int sender_id) {
  const bool self_isolated = cur_fail.find(self_id) != cur_fail.end();
  const bool sender_isolated = cur_fail.find(sender_id) != cur_fail.end();
  return self_isolated != sender_isolated;
}

}  

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)){
  int total_replicas = config_.GetReplicaNum();
  int f = (total_replicas - 1) / 3;
  f_ = f;
  id_ = config_.GetSelfInfo().id();

  Init();

  start_ = 0;

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    cassandra_ = std::make_unique<cassandra_recv::Cassandra>(
        config_.GetSelfInfo().id(), f,
                                   total_replicas, config_.GetConfigData().block_size(), 
                                   GetSignatureVerifier());

  global_stats_ = Stats::GetGlobalStats();
    InitProtocol(cassandra_.get());


    cassandra_->SetPrepareFunction([&](const Transaction& msg) { 
      return Prepare(msg); 
    });
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  // Per-message receive log was duplicated (before and after the partition-
  // isolation check) and fired on every consensus message arrival -- in 32
  // node clusters that's tens of thousands of synchronous LOG(ERROR) writes
  // per second per replica, which is the dominant cause of stderr-flood and
  // SIGKILL we observed on the slow replicas. Suppressed entirely.
  if (EnableGroupMove()) {
    std::unique_lock<std::mutex> lk(g_mutex_);
    if (!g_group_move_state.started) {
      g_group_move_state.started = true;
      cur_fail.clear();
      StartGroupMoveThread(config_.GetReplicaNum());
    }

    if (IsCrossPartitionBlocked(config_.GetSelfInfo().id(), request->sender_id())) {
      // Per-message isolation drop log suppressed -- during partition this
      // fires on every cross-partition message and dwarfs every other log.
      global_stats_->SeqGap(cur_fail.size());
      return 0;
    }
    global_stats_->SeqGap(cur_fail.size());
  }


  if (request->user_type() == MessageType::NewBlocks) {
    std::unique_ptr<Block> block = std::make_unique<Block>();
    if (!block->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveBlock(std::move(block));
    return 0;
  } else if (request->user_type() == MessageType::CMD_ProposalVote) {
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    cassandra_->ReceiveProposalVote(std::move(proposal));
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockACK) {
    std::unique_ptr<BlockACK> block_ack = std::make_unique<BlockACK>();
    if (!block_ack->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    cassandra_->ReceiveBlockACK(std::move(block_ack));
    return 0;

  } else if (request->user_type() == MessageType::NewProposal) {
    // LOG(ERROR)<<"receive proposal:";
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!cassandra_->ReceiveProposal(std::move(proposal))) {
      return -1;
    }
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockQuery) {
    std::unique_ptr<BlockQuery> block = std::make_unique<BlockQuery>();
    if (!block->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveAskBlock(std::move(block));
    return 0;
  } else if (request->user_type() == MessageType::CMD_ProposalQuery) {
    std::unique_ptr<Block> query =
      std::make_unique<Block>();
    if (!query->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveAskBlockAck(std::move(query));
  } else if (request->user_type() ==
      MessageType::CMD_ProposalQueryResponse) {
    std::unique_ptr<ProposalQueryResp> resp =
      std::make_unique<ProposalQueryResp>();
    if (!resp->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    //cassandra_->ReceiveProposalQueryResp(*resp);
  } else if (request->user_type() ==
      MessageType::CMD_AskProposal) {
    std::unique_ptr<ProposalQuery> resp =
      std::make_unique<ProposalQuery>();
    if (!resp->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    cassandra_->ReceiveAskProposal(std::move(resp));
  } else if (request->user_type() ==
      MessageType::CMD_AskProposalAck) {
    std::unique_ptr<ProposalQueryResp> resp =
      std::make_unique<ProposalQueryResp>();
    if (!resp->ParseFromString(request->data())) {
      assert(1 == 0);
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    // resp diag suppressed: hot path -- every AskProposalAck inbound message
    // (~hundreds per second per replica during recovery).
    cassandra_->ReceiveAskProposalAck(std::move(resp));
  } 

  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  //LOG(ERROR)<<"receive txn";
  return cassandra_->ReceiveTransaction(std::move(txn));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  //LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<" uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_uid(txn.uid());
  //if (txn.proposer_id() == config_.GetSelfInfo().id()) {
    request->set_proxy_id(txn.proxy_id());
   // LOG(ERROR)<<"commit txn:"<<txn.id()<<" proxy id:"<<request->uid();
    //assert(request->uid()>0);
  //}

  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}


int Consensus::Prepare(const Transaction& txn) {
  // LOG(ERROR)<<"prepare txn:"<<txn.id()<<" proxy id:"<<txn.proxy_id()<<"
  // uid:"<<txn.uid();
  std::unique_ptr<Request> request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_uid(txn.uid());
  transaction_executor_->Prepare(std::move(request));
  return 0;
}


}  // namespace cassandra
}  // namespace resdb
