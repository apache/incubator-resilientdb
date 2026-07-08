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

#include "platform/consensus/ordering/rcc/framework/consensus.h"

#include <cstdlib>
#include <functional>
#include <glog/logging.h>
#include <set>
#include <thread>
#include <unistd.h>
#include <vector>

#include "common/utils/utils.h"

namespace resdb {
namespace rcc {
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

}  // namespace

Consensus::Consensus(const ResDBConfig& config,
                     std::unique_ptr<TransactionManager> executor)
    : common::Consensus(config, std::move(executor)) {
  const int total_replicas = config_.GetReplicaNum();
  const int f = (total_replicas - 1) / 3;

  Init();
  global_stats_ = Stats::GetGlobalStats();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    rcc_ = std::make_unique<RCC>(config_.GetSelfInfo().id(), f, total_replicas,
                                 config_.GetConfigData().block_size(),
                                 GetSignatureVerifier());

    rcc_->SetSingleCallFunc(
        [&](int type, const google::protobuf::Message& msg, int node_id) {
          return SendMsg(type, msg, node_id);
        });

    rcc_->SetBroadcastCallFunc(
        [&](int type, const google::protobuf::Message& msg) {
          return Broadcast(type, msg);
        });

    rcc_->SetCommitFunc([&](const google::protobuf::Message& msg) {
      return CommitMsg(dynamic_cast<const Transaction&>(msg));
    });
  }
}

int Consensus::ProcessCustomConsensus(std::unique_ptr<Request> request) {
  if (EnableGroupMove()) {
    std::unique_lock<std::mutex> lk(g_mutex_);
    if (!g_group_move_state.started) {
      g_group_move_state.started = true;
      cur_fail.clear();
      StartGroupMoveThread(config_.GetReplicaNum());
    }

    if (IsCrossPartitionBlocked(config_.GetSelfInfo().id(), request->sender_id())) {
      LOG(ERROR) << "skip message by isolation, sender:" << request->sender_id()
                 << " self:" << config_.GetSelfInfo().id();
      global_stats_->SeqGap(cur_fail.size());
      return 0;
    }
    global_stats_->SeqGap(cur_fail.size());
  }

  if (request->user_type() == MessageType::ConsensusMsg) {
    Proposal proposal;
    if (!proposal.ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    return rcc_->ReceiveProposal(proposal) ? 0 : -1;
  }

  if (request->user_type() == MessageType::SeqSync) {
    SyncMsg proposal;
    if (!proposal.ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal list fail";
      return -1;
    }
    rcc_->ReceiveSyncMsg(proposal);
  }

  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  return rcc_->ReceiveTransaction(std::move(txn));
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  auto request = std::make_unique<Request>();
  request->set_queuing_time(txn.queuing_time());
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace rcc
}  // namespace resdb
