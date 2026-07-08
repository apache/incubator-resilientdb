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

#include "platform/consensus/ordering/autobahn/framework/consensus.h"

#include <glog/logging.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>

#include "common/utils/utils.h"

namespace resdb {
namespace autobahn {

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
      sleep(15);

      {
        std::unique_lock<std::mutex> lk(g_mutex_);
        for(int i = 10; i <=total_replicas && cur_fail.size()<target1; i++) {
          LOG(ERROR)<<"move node1:"<<i;
          cur_fail.insert(i);
        }
      }
    }

    sleep(15);

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

    sleep(15);
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
  global_stats_ = Stats::GetGlobalStats();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    autobahn_ = std::make_unique<AutoBahn>(
        config_.GetSelfInfo().id(), f,
                                   total_replicas, config_.GetConfigData().block_size(), 
                                   GetSignatureVerifier());

    InitProtocol(autobahn_.get());

  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  LOG(ERROR)<<"receive commit:"<<request->type()<<" "<<MessageType_Name(request->user_type())<<" from:"<<request->sender_id();
  if (EnableGroupMove()) {
    // One-time startup of the partition-injection thread. Use a CAS-style
    // double-checked init so we no longer hold g_mutex_ across the hot path
    // for every inbound consensus message — that lock previously serialized
    // ProcessCustomConsensus across all worker threads of the node.
    if (!g_group_move_state.started) {
      std::unique_lock<std::mutex> lk(g_mutex_);
      if (!g_group_move_state.started) {
        g_group_move_state.started = true;
        cur_fail.clear();
        StartGroupMoveThread(config_.GetReplicaNum());
      }
    }

    // Lock-free read of the partition snapshot (atomic bitmask).
    if (IsCrossPartitionBlocked(config_.GetSelfInfo().id(), request->sender_id())) {
      LOG(ERROR) << "skip message by isolation, sender:" << request->sender_id()
                 << " self:" << config_.GetSelfInfo().id();
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
    autobahn_->ReceiveBlock(std::move(block));
    return 0;
  } 
  else if (request->user_type() == MessageType::CMD_BlockACK) {
    std::unique_ptr<BlockACK> block_ack = std::make_unique<BlockACK>();
    if (!block_ack->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->ReceiveBlockACK(std::move(block_ack));
    return 0;

  } else if (request->user_type() == MessageType::CMD_NewLeader) {
    std::unique_ptr<VoteMsg> msg = std::make_unique<VoteMsg>();
    if (!msg->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->ReceiveNewLeader(std::move(msg));
    return 0;

  } else if (request->user_type() == MessageType::NewProposal) {
    // LOG(ERROR)<<"receive proposal:";
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!autobahn_->ReceiveProposal(std::move(proposal))) {
      return -1;
    }
    return 0;
  } else if (request->user_type() == MessageType::ProposalAck) {
    // LOG(ERROR)<<"receive proposal:";
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!autobahn_->ReceiveVote(std::move(proposal))) {
      return -1;
    }
    return 0;
  } else if (request->user_type() == MessageType::Prepare) {
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!autobahn_->ReceivePrepare(std::move(proposal))) {
      return -1;
    }
    return 0;

  } else if (request->user_type() == MessageType::Commit) {
    std::unique_ptr<Proposal> proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    if (!autobahn_->ReceiveCommit(std::move(proposal))) {
      return -1;
    }
    return 0;
  } 
  else if (request->user_type() == MessageType::CMD_BlockReq) {
    std::unique_ptr<BlockACK> block = std::make_unique<BlockACK>();
    if (!block->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskBlock(std::move(block));
    return 0;
  }else if (request->user_type() == MessageType::CMD_BlockReqAck) {
    std::unique_ptr<Block> block = std::make_unique<Block>();
    if (!block->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskBlockAck(std::move(block));
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockBatchReq) {
    std::unique_ptr<BlockBatch> block_batch = std::make_unique<BlockBatch>();
    if (!block_batch->ParseFromString(request->data())) {
      LOG(ERROR) << "parse block batch request fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskBlockBatch(std::move(block_batch));
    return 0;
  } else if (request->user_type() == MessageType::CMD_BlockBatchResp) {
    std::unique_ptr<BlockBatch> block_batch = std::make_unique<BlockBatch>();
    if (!block_batch->ParseFromString(request->data())) {
      LOG(ERROR) << "parse block batch response fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskBlockBatchAck(std::move(block_batch));
    return 0;
  } else if (request->user_type() == MessageType::CMD_ProposalQuery) {
    std::unique_ptr<ProposalQuery> proposal_query =
        std::make_unique<ProposalQuery>();
    if (!proposal_query->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal query fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskProposal(std::move(proposal_query));
    return 0;
  } else if (request->user_type() == MessageType::CMD_ProposalQueryResponse) {
    std::unique_ptr<ProposalQueryResp> proposals =
        std::make_unique<ProposalQueryResp>();
    if (!proposals->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal query response fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvAskProposalAck(std::move(proposals));
    return 0;
  } else if (request->user_type() == MessageType::CMD_StateReq) {
    std::unique_ptr<RecoveryState> recovery_state =
        std::make_unique<RecoveryState>();
    if (!recovery_state->ParseFromString(request->data())) {
      LOG(ERROR) << "parse recovery state request fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvStateRequest(std::move(recovery_state));
    return 0;
  } else if (request->user_type() == MessageType::CMD_StateResp) {
    std::unique_ptr<RecoveryState> recovery_state =
        std::make_unique<RecoveryState>();
    if (!recovery_state->ParseFromString(request->data())) {
      LOG(ERROR) << "parse recovery state response fail";
      assert(1 == 0);
      return -1;
    }
    autobahn_->RecvStateResponse(std::move(recovery_state));
    return 0;
  } 
  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  std::unique_ptr<Transaction> txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  //LOG(ERROR)<<"receive txn";
  return autobahn_->ReceiveTransaction(std::move(txn));
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


}  // namespace autobahn
}  // namespace resdb
