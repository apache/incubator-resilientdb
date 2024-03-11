#pragma once

#include <deque>
#include <map>
#include <queue>
#include <thread>

#include "platform/common/queue/lock_free_queue.h"
#include "platform/consensus/ordering/common/algorithm/protocol_base.h"
#include "platform/consensus/ordering/poe/proto/proposal.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace poe {

class PoE: public common::ProtocolBase {
 public:
  PoE(int id, int f, int total_num, SignatureVerifier* verifier);
  ~PoE();

  bool ReceiveTransaction(std::unique_ptr<Transaction> txn);
  bool ReceivePropose(std::unique_ptr<Transaction> txn);
  bool ReceivePrepare(std::unique_ptr<Proposal> proposal);

 private:
  bool IsStop();

 private:
  std::mutex mutex_;
  std::map<std::string, std::set<int32_t> > received_;
  std::map<std::string, std::unique_ptr<Transaction> > data_;

  int64_t seq_;
 bool is_stop_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
};

}  // namespace cassandra
}  // namespace resdb
