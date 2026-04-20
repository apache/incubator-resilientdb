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

#include "platform/consensus/ordering/raft/algorithm/leaderelection_manager.h"
#include "platform/consensus/ordering/raft/algorithm/raft.h"
#include <glog/logging.h>
#include <chrono>
#include <thread>

#include "common/utils/utils.h"
#include "platform/proto/viewchange_message.pb.h"

namespace resdb {
namespace raft {

// A manager to address View change process.
// All stuff here will be addressed in sequential by using mutex
// to make things simplier.
LeaderElectionManager::LeaderElectionManager(const ResDBConfig& config)
    : config_(config),
      raft_(nullptr),
      started_(false),
      stop_(false),
      timeout_min_ms(1200),
      timeout_max_ms(2400),
      heartbeat_timer_(100),
      heartbeat_count_(0),
      //last_heartbeat_time_(std::chrono::steady_clock::now()),
      broadcast_count_(0),
      role_epoch_(0),
      known_role_epoch_(0) {
  global_stats_ = Stats::GetGlobalStats();
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": in LeaderElectionManager constructor";
}

LeaderElectionManager::~LeaderElectionManager() {
  stop_.store(true);
  cv_.notify_all();

  if (server_checking_timeout_thread_.joinable()) {
    server_checking_timeout_thread_.join();
  }
}

void LeaderElectionManager::MayStart() {
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": in LeaderElectionManager MayStart";
  bool expected = false;
    if (!started_.compare_exchange_strong(expected, true)) {
        return;
    }

  if (config_.GetPublicKeyCertificateInfo()
          .public_key()
          .public_key_info()
          .type() == CertificateKeyInfo::CLIENT) {
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": in LeaderElectionManager MayStart, Client conditional";
    LOG(ERROR) << "client type not process view change";
    return;
  }

  if (config_.GetConfigData().enable_viewchange()) {
    //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Starting MonitoringElectionTimeout thread.";
    server_checking_timeout_thread_ =
        std::thread(&LeaderElectionManager::MonitoringElectionTimeout, this);
  }
}

void LeaderElectionManager::SetRaft(raft::Raft* raft) {
  raft_ = raft;
}

void LeaderElectionManager::OnHeartBeat() {
    //auto now = std::chrono::steady_clock::now();
    //std::chrono::steady_clock::duration delta;
  {
    std::lock_guard<std::mutex> lk(cv_mutex_);
    heartbeat_count_++;
    //delta = now - last_heartbeat_time_;
    //last_heartbeat_time_ = now;
  }
  cv_.notify_all();
  //auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
  //LOG(INFO) << "JIM -> " << __FUNCTION__ << ": Heartbeat received after " << ms << "ms";
}

void LeaderElectionManager::OnRoleChange() {
  {
    LOG(INFO) << "JIM -> " << __FUNCTION__;
    std::lock_guard<std::mutex> lk(cv_mutex_);
    role_epoch_++;
  }
  cv_.notify_all();
}

void LeaderElectionManager::OnAeBroadcast() {
  {
    LOG(INFO) << "JIM -> " << __FUNCTION__;
    std::lock_guard<std::mutex> lk(cv_mutex_);
    broadcast_count_++;
  }
  cv_.notify_all();
}

uint64_t LeaderElectionManager::RandomInt(uint64_t min, uint64_t max) {
    static thread_local std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(min, max);
    return dist(gen);
}

Waited LeaderElectionManager::LeaderWait() {
  //LOG(INFO) << "JIM -> " << __FUNCTION__;
  std::unique_lock<std::mutex> lk(cv_mutex_);
  const uint64_t broadcast_snapshot = broadcast_count_;
  if (known_role_epoch_ != role_epoch_) {
    known_role_epoch_ = role_epoch_;
    return Waited::ROLE_CHANGE;
  }
  cv_.wait_for(lk, std::chrono::milliseconds(heartbeat_timer_),
              [this, broadcast_snapshot] {
                  return (stop_.load() == true
                  || (known_role_epoch_ != role_epoch_)
                  || (broadcast_snapshot != broadcast_count_));
                });
  if (stop_.load() == true) {
    return Waited::STOPPED;
  }
  else if (known_role_epoch_ != role_epoch_) { 
    known_role_epoch_ = role_epoch_;
    return Waited::ROLE_CHANGE; 
  }
  else if (broadcast_snapshot != broadcast_count_) {
    return Waited::BROADCASTED;
  }
  else {
    return Waited::TIMEOUT;
  }
}

Waited LeaderElectionManager::Wait() {
  //LOG(INFO) << "JIM -> " << __FUNCTION__;
  const uint64_t timeout_ms = RandomInt(timeout_min_ms, timeout_max_ms);
  timeout_ms_ = timeout_ms;
  std::unique_lock<std::mutex> lk(cv_mutex_);
  const uint64_t heartbeat_snapshot = heartbeat_count_;
  if (known_role_epoch_ != role_epoch_) {
    known_role_epoch_ = role_epoch_;
    return Waited::ROLE_CHANGE;
  }
  cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
              [this, heartbeat_snapshot] {
                  return (stop_.load() == true
                  || (heartbeat_snapshot != heartbeat_count_)
                  || (known_role_epoch_ != role_epoch_));
                });
  if (stop_.load() == true) {
    return Waited::STOPPED;
  }
  else if (known_role_epoch_ != role_epoch_) { 
    known_role_epoch_ = role_epoch_;
    return Waited::ROLE_CHANGE; 
  }
  else if (heartbeat_snapshot != heartbeat_count_) {
    return Waited::HEARTBEAT;
  }
  else {
    return Waited::TIMEOUT;
  }
}

// Function that is run in server_checking_timeout_thread started in MayStart().
// Causes leaders to Heartbeat.
// Causes followers and candidates to start an election if no heartbeat received.
void LeaderElectionManager::MonitoringElectionTimeout() {
  while (!stop_.load()) {
    Role role = raft_->GetRoleSnapshot();
    Waited res;
    std::chrono::steady_clock::time_point wait_start_time_ = std::chrono::steady_clock::now();
    bool leader = false;
    if (role == Role::LEADER) { 
      res = LeaderWait(); 
      leader = true;
    }
    else { 
      res = Wait(); 
    }
    std::chrono::steady_clock::time_point wait_end_time_ = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration delta = wait_end_time_ - wait_start_time_;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    if (raft_->livenessLoggingFlag_) {
      LOG(INFO) << __FUNCTION__ << ": " << (leader ? "Leader" : "") << "Wait " << ms << "ms";
    }
    if (res == Waited::STOPPED) {
      break;
    }
    else if (res == Waited::ROLE_CHANGE) {
      LOG(INFO) << __FUNCTION__ << ": Role change detected";
      continue; 
    }
    else if (res == Waited::HEARTBEAT) {
      //LOG(INFO) << __FUNCTION__ << ": Heartbeat received within window";
      if (raft_->GetRoleSnapshot() == Role::LEADER) {
        // A leader receiving a heartbeat would be unusual but not impossible.
        LOG(WARNING) << __FUNCTION__ << " Received Heartbeat as LEADER";
      }
      continue;
    }
    else if (res == Waited::BROADCASTED) {
      if (raft_->livenessLoggingFlag_) {
        LOG(INFO) << __FUNCTION__ << ": AE Broadcast reset leader heartbeat timer";
      }
      continue; 
    }
    
    // Only gets here if timeout expired.
    // Leaders send a new heartbeat.
    if (raft_->GetRoleSnapshot() == Role::LEADER) {
        raft_->SendHeartBeat();
    }
    // Followers and Candidates start an election.
    else {
      LOG(INFO) << __FUNCTION__ << ": Heartbeat timed out after " << timeout_ms_.load() << " ms";
      raft_->StartElection();
    }
  }
}

} // namespace raft
}  // namespace resdb
