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

#pragma once

#include "platform/common/queue/blocking_queue.h"
#include "platform/config/resdb_poc_config.h"
#include "platform/consensus/ordering/poc/pow/block_manager.h"
#include "platform/consensus/ordering/poc/pow/miner_manager.h"
#include "platform/consensus/ordering/poc/pow/shift_manager.h"
#include "platform/consensus/ordering/poc/pow/transaction_accessor.h"
#include "platform/consensus/ordering/poc/proto/pow.pb.h"
#include "platform/networkstrate/replica_communicator.h"

namespace resdb {

class PoWManager {
 public:
  PoWManager(const ResDBPoCConfig& config, ReplicaCommunicator* bc_client);
  virtual ~PoWManager();

  void Start();
  void Stop();
  bool IsRunning();
  void Reset();
  void Commit(std::unique_ptr<Block> block);
  void AddShiftMsg(const SliceInfo& slice_info);

  enum MiningStatus {
    OK = 0,
    TIMEOUT = 1,
    FAIL = 2,
  };

  enum BlockStatus {
    GENERATE_NEW = 0,
    NEXT_NEWBLOCK = 1,
  };

  enum MiningType {
    NEWBLOCK = 0,
    SHIFTING = 1,
  };

 protected:
  virtual std::unique_ptr<TransactionAccessor> GetTransactionAccessor(
      const ResDBPoCConfig& config);
  virtual std::unique_ptr<ShiftManager> GetShiftManager(
      const ResDBPoCConfig& config);
  virtual std::unique_ptr<BlockManager> GetBlockManager(
      const ResDBPoCConfig& config);

  virtual MiningStatus Wait();
  virtual void NotifyBroadCast();
  virtual int GetShiftMsg(const SliceInfo& slice_info);

  int GetMiningTxn(MiningType type);
  void NotifyNextBlock();
  absl::Status WaitBlockDone();
  BlockStatus GetBlockStatus();

  void SendShiftMsg();
  void MiningProcess();
  int BroadCastNewBlock(const Block& block);
  int BroadCastShiftMsg(const SliceInfo& slice_info);
  void GossipProcess();

 private:
  ResDBPoCConfig config_;
  std::unique_ptr<BlockManager> block_manager_;
  std::unique_ptr<ShiftManager> shift_manager_;
  std::unique_ptr<TransactionAccessor> transaction_accessor_;
  std::thread miner_thread_, gossip_thread_;
  std::atomic<bool> is_stop_;

  std::mutex broad_cast_mtx_, mutex_, tx_mutex_;
  std::condition_variable broad_cast_cv_, cv_;
  std::atomic<BlockStatus> current_status_ = BlockStatus::GENERATE_NEW;
  ReplicaCommunicator* bc_client_;
  SliceInfo need_slice_info_;
  PrometheusHandler* prometheus_handler_;
};

}  // namespace resdb
