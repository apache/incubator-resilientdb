#include "platform/consensus/ordering/hotstuff_1/framework/consensus.h"

#include <cstdlib>
#include <glog/logging.h>
#include <set>
#include <thread>
#include <unistd.h>
#include <vector>

#include "common/utils/utils.h"

namespace resdb {
namespace hotstuff_1 {

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
  Init();

  const int total_replicas = config_.GetReplicaNum();
  f_ = (total_replicas - 1) / 3;
  id_ = config_.GetSelfInfo().id();
  global_stats_ = Stats::GetGlobalStats();

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() != CertificateKeyInfo::CLIENT) {
    hotstuff_ = std::make_unique<HotStuff>(config_.GetSelfInfo().id(), f_,
                                           total_replicas,
                                           config_.GetConfigData().block_size(),
                                           GetSignatureVerifier());
    InitProtocol(hotstuff_.get());
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

  LOG(ERROR) << "recv request:" << MessageType_Name(request->user_type())
             << " from:" << request->sender_id();

  if (request->user_type() == MessageType::NewProposal) {
    auto proposal = std::make_unique<Proposal>();
    if (!proposal->ParseFromString(request->data())) {
      LOG(ERROR) << "parse proposal fail";
      return -1;
    }
    return hotstuff_->ReceiveProposal(std::move(proposal)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::Vote) {
    auto cert = std::make_unique<Certificate>();
    if (!cert->ParseFromString(request->data())) {
      LOG(ERROR) << "parse cert fail";
      return -1;
    }
    return hotstuff_->ReceiveCertificate(std::move(cert)) ? 0 : -1;
  }

  if (request->user_type() == MessageType::StartViewMsg) {
    auto start_view = std::make_unique<StartView>();
    if (!start_view->ParseFromString(request->data())) {
      LOG(ERROR) << "parse start view fail";
      return -1;
    }
    return hotstuff_->ReceiveStartView(std::move(start_view)) ? 0 : -1;
  }

  return 0;
}

int Consensus::ProcessNewTransaction(std::unique_ptr<Request> request) {
  auto txn = std::make_unique<Transaction>();
  txn->set_data(request->data());
  txn->set_hash(request->hash());
  txn->set_proxy_id(request->proxy_id());
  txn->set_user_seq(request->user_seq());
  return hotstuff_->ReceiveTransaction(std::move(txn)) ? 0 : -1;
}

int Consensus::CommitMsg(const google::protobuf::Message& msg) {
  return CommitMsgInternal(dynamic_cast<const Transaction&>(msg));
}

int Consensus::CommitMsgInternal(const Transaction& txn) {
  auto request = std::make_unique<Request>();
  request->set_data(txn.data());
  request->set_seq(txn.id());
  request->set_proxy_id(txn.proxy_id());
  transaction_executor_->AddExecuteMessage(std::move(request));
  return 0;
}

}  // namespace hotstuff_1
}  // namespace resdb
