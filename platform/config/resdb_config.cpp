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

#include "platform/config/resdb_config.h"

#include <glog/logging.h>

namespace resdb {

ResDBConfig::ResDBConfig(const std::vector<ReplicaInfo>& replicas,
                         const ReplicaInfo& self_info,
                         ResConfigData config_data)
    : ResDBConfig(config_data, self_info, KeyInfo(), CertificateInfo()) {
  replicas_ = replicas;
}

ResDBConfig::ResDBConfig(const std::vector<ReplicaInfo>& replicas,
                         const ReplicaInfo& self_info,
                         const KeyInfo& private_key,
                         const CertificateInfo& public_key_cert_info)
    : ResDBConfig(ResConfigData(), self_info, private_key,
                  public_key_cert_info) {
  replicas_ = replicas;
}

ResDBConfig::ResDBConfig(const ResConfigData& config_data,
                         const ReplicaInfo& self_info,
                         const KeyInfo& private_key,
                         const CertificateInfo& public_key_cert_info)
    : config_data_(config_data),
      self_info_(self_info),
      private_key_(private_key),
      public_key_cert_info_(public_key_cert_info) {
  for (const auto& region : config_data.region()) {
    if (region.region_id() == config_data.self_region_id()) {
      LOG(INFO) << "get region info:" << region.DebugString();
      for (const auto& replica : region.replica_info()) {
        replicas_.push_back(replica);
      }
      LOG(INFO) << "get region config server size:"
                << region.replica_info_size();
      break;
    }
  }
  if (config_data_.view_change_timeout_ms() == 0) {
    config_data_.set_view_change_timeout_ms(viewchange_commit_timeout_ms_);
  }
  if (config_data_.client_batch_num() == 0) {
    config_data_.set_client_batch_num(client_batch_num_);
  }
  if (config_data_.worker_num() == 0) {
    config_data_.set_worker_num(worker_num_);
  }
  if (config_data_.input_worker_num() == 0) {
    config_data_.set_input_worker_num(input_worker_num_);
  }
  if (config_data_.output_worker_num() == 0) {
    config_data_.set_output_worker_num(output_worker_num_);
  }
  if (config_data_.tcp_batch_num() == 0) {
    config_data_.set_tcp_batch_num(100);
  }
}

void ResDBConfig::SetConfigData(const ResConfigData& config_data) {
  config_data_ = config_data;
  replicas_.clear();
  for (const auto& region : config_data.region()) {
    if (region.region_id() == config_data.self_region_id()) {
      LOG(INFO) << "get region info:" << region.DebugString();
      for (const auto& replica : region.replica_info()) {
        replicas_.push_back(replica);
      }
      LOG(INFO) << "get region config server size:"
                << region.replica_info_size();
      break;
    }
  }
  if (config_data_.view_change_timeout_ms() == 0) {
    config_data_.set_view_change_timeout_ms(viewchange_commit_timeout_ms_);
  }
}

KeyInfo ResDBConfig::GetPrivateKey() const { return private_key_; }

CertificateInfo ResDBConfig::GetPublicKeyCertificateInfo() const {
  return public_key_cert_info_;
}

ResConfigData ResDBConfig::GetConfigData() const { return config_data_; }

const std::vector<ReplicaInfo>& ResDBConfig::GetReplicaInfos() const {
  return replicas_;
}

const ReplicaInfo& ResDBConfig::GetSelfInfo() const { return self_info_; }

size_t ResDBConfig::GetReplicaNum() const { return replicas_.size(); }

int ResDBConfig::GetMinDataReceiveNum() const {
  int f = (replicas_.size() - 1) / 3;
  return std::max(2 * f + 1, 1);
}

int ResDBConfig::GetMinClientReceiveNum() const {
  int f = (replicas_.size() - 1) / 3;
  return std::max(f + 1, 1);
}

int ResDBConfig::GetMinCheckpointReceiveNum() const {
  int f = (replicas_.size() - 1) / 3;
  return std::max(f + 1, 1);
}

size_t ResDBConfig::GetMaxMaliciousReplicaNum() const {
  int f = (replicas_.size() - 1) / 3;
  return std::max(f, 0);
}

void ResDBConfig::SetClientTimeoutMs(int timeout_ms) {
  client_timeout_ms_ = timeout_ms;
}

int ResDBConfig::GetClientTimeoutMs() const { return client_timeout_ms_; }

// Logging
std::string ResDBConfig::GetCheckPointLoggingPath() const {
  return checkpoint_logging_path_;
}

void ResDBConfig::SetCheckPointLoggingPath(const std::string& path) {
  checkpoint_logging_path_ = path;
}

int ResDBConfig::GetCheckPointWaterMark() const {
  return checkpoint_water_mark_;
}

void ResDBConfig::SetCheckPointWaterMark(int water_mark) {
  checkpoint_water_mark_ = water_mark;
}

void ResDBConfig::EnableCheckPoint(bool is_enable) {
  is_enable_checkpoint_ = is_enable;
}

bool ResDBConfig::IsCheckPointEnabled() { return is_enable_checkpoint_; }

bool ResDBConfig::HeartBeatEnabled() { return hb_enabled_; }

void ResDBConfig::SetHeartBeatEnabled(bool enable_heartbeat) {
  hb_enabled_ = enable_heartbeat;
}

bool ResDBConfig::SignatureVerifierEnabled() {
  return signature_verifier_enabled_;
}

void ResDBConfig::SetSignatureVerifierEnabled(bool enable_sv) {
  signature_verifier_enabled_ = enable_sv;
}

// Performance setting
bool ResDBConfig::IsPerformanceRunning() {
  return is_performance_running_ || GetConfigData().is_performance_running();
}

void ResDBConfig::RunningPerformance(bool is_performance_running) {
  is_performance_running_ = is_performance_running;
}

void ResDBConfig::SetTestMode(bool is_test_mode) {
  is_test_mode_ = is_test_mode;
}

bool ResDBConfig::IsTestMode() const { return is_test_mode_; }

uint32_t ResDBConfig::GetMaxProcessTxn() const {
  if (config_data_.max_process_txn()) {
    return config_data_.max_process_txn();
  }
  return max_process_txn_;
}

void ResDBConfig::SetMaxProcessTxn(uint32_t num) {
  config_data_.set_max_process_txn(num);
  max_process_txn_ = num;
}

uint32_t ResDBConfig::GetMaxClientComplaintNum() const {
  if (config_data_.max_client_complaint_num()) {
    return config_data_.max_client_complaint_num();
  }
  return 10;
}

uint32_t ResDBConfig::ClientBatchWaitTimeMS() const {
  return client_batch_wait_time_ms_;
}

void ResDBConfig::SetClientBatchWaitTimeMS(uint32_t wait_time_ms) {
  client_batch_wait_time_ms_ = wait_time_ms;
}

uint32_t ResDBConfig::ClientBatchNum() const {
  return config_data_.client_batch_num();
}

void ResDBConfig::SetClientBatchNum(uint32_t num) {
  config_data_.set_client_batch_num(num);
}

uint32_t ResDBConfig::GetWorkerNum() const { return config_data_.worker_num(); }

uint32_t ResDBConfig::GetInputWorkerNum() const {
  return config_data_.input_worker_num();
}

uint32_t ResDBConfig::GetOutputWorkerNum() const {
  return config_data_.output_worker_num();
}

uint32_t ResDBConfig::GetTcpBatchNum() const {
  return config_data_.tcp_batch_num();
}

uint32_t ResDBConfig::GetViewchangeCommitTimeout() const {
  return config_data_.view_change_timeout_ms()
             ? config_data_.view_change_timeout_ms()
             : viewchange_commit_timeout_ms_;
}

void ResDBConfig::SetViewchangeCommitTimeout(uint64_t timeout_ms) {
  config_data_.set_view_change_timeout_ms(timeout_ms);
}

}  // namespace resdb
