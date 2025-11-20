/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <semaphore.h>

#include "platform/config/resdb_config.h"
#include "platform/consensus/execution/system_info.h"
#include "platform/proto/viewchange_message.pb.h"
#include "platform/statistic/stats.h"

namespace resdb {
namespace raft {

class Raft; // forward declaration

enum class Waited {
  HEARTBEAT,
  STOPPED,
  TIMEOUT,
  ROLE_CHANGE
};

class LeaderElectionManager {
 public:
  LeaderElectionManager(const ResDBConfig& config);
  virtual ~LeaderElectionManager();

  // If the monitor is not running, start to monitor.
  void MayStart();
  void SetRaft(raft::Raft*);
  void OnHeartBeat();
  void OnRoleChange();

 private:
  Waited LeaderWait();
  Waited Wait();
  void MonitoringElectionTimeout();
  uint64_t RandomInt(uint64_t min, uint64_t max);
  

 protected:
  ResDBConfig config_;
  Stats* global_stats_;
  raft::Raft* raft_;
  std::map<uint64_t, std::map<uint32_t, ViewChangeMessage>> viewchange_request_;
  std::atomic<bool> started_;
  std::atomic<bool> stop_;
  std::thread server_checking_timeout_thread_;
  uint64_t timeout_ms_;
  uint64_t timeout_min_ms;
  uint64_t timeout_max_ms;
  uint64_t heartbeat_timer_;
  uint64_t heartbeat_count_; // Protected by cv_mutex_
  uint64_t role_epoch_; // Protected by cv_mutex_
  uint64_t known_role_epoch_; // Protected by cv_mutex_
  std::mutex cv_mutex_;
  std::condition_variable cv_;
};

}  // namespace raft
}  // namespace resdb

