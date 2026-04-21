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

#include "sidecar/migration_daemon/migration_daemon.h"

#include <glog/logging.h>
#include <thread>

namespace resdb {
namespace storage {
namespace sidecar {

namespace {

constexpr int kDefaultPollIntervalSeconds = 60;
constexpr int kDefaultBatchSize = 1000;

}

MigrationDaemon::MigrationDaemon(
    std::unique_ptr<Storage> storage,
    std::unique_ptr<IPFSClient> ipfs_client,
    const TieredStorageConfig& config)
    : storage_(std::move(storage)),
      ipfs_client_(std::move(ipfs_client)),
      config_(config),
      running_(false),
      migrated_keys_(0),
      last_checkpoint_(0) {
  if (config_.poll_interval_seconds() <= 0) {
    config_.set_poll_interval_seconds(kDefaultPollIntervalSeconds);
  }
  if (config_.batch_size() <= 0) {
    config_.set_batch_size(kDefaultBatchSize);
  }
  
  LOG(INFO) << "MigrationDaemon initialized: "
           << "poll_interval=" << config_.poll_interval_seconds() << "s, "
           << "batch_size=" << config_.batch_size() << ", "
           << "cold_threshold=" << config_.cold_threshold_checkpoint();
}

MigrationDaemon::~MigrationDaemon() {
  Stop();
}

void MigrationDaemon::Start() {
  if (running_.load()) {
    LOG(WARNING) << "MigrationDaemon already running";
    return;
  }
  
  if (!ipfs_client_ || !ipfs_client_->IsEnabled()) {
    LOG(WARNING) << "IPFS client not enabled, cannot start migration daemon";
    return;
  }
  
  running_.store(true);
  poll_thread_ = std::thread(&MigrationDaemon::PollLoop, this);
  LOG(INFO) << "MigrationDaemon started";
}

void MigrationDaemon::Stop() {
  if (!running_.load()) {
    return;
  }
  
  running_.store(false);
  if (poll_thread_.joinable()) {
    poll_thread_.join();
  }
  LOG(INFO) << "MigrationDaemon stopped";
}

void MigrationDaemon::RunMigration() {
  if (!running_.load()) {
    LOG(INFO) << "MigrationDaemon not running, skipping migration";
    return;
  }
  
  MigrateDataOlderThanThreshold();
}

void MigrationDaemon::PollLoop() {
  LOG(INFO) << "MigrationDaemon polling started";
  
  while (running_.load()) {
    try {
      last_checkpoint_ = storage_->GetLastCheckpoint();
      LOG(INFO) << "Current checkpoint: " << last_checkpoint_;
      
      MigrateDataOlderThanThreshold();
    } catch (const std::exception& e) {
      LOG(ERROR) << "MigrationDaemon error: " << e.what();
    }
    
    std::this_thread::sleep_for(
        std::chrono::seconds(config_.poll_interval_seconds()));
  }
  
  LOG(INFO) << "MigrationDaemon polling stopped";
}

bool MigrationDaemon::MigrateDataOlderThanThreshold() {
  uint64_t threshold = config_.cold_threshold_checkpoint();
  if (last_checkpoint_ <= threshold) {
    LOG(INFO) << "Checkpoint " << last_checkpoint_ 
             << " not past threshold " << threshold;
    return false;
  }
  
  uint64_t cold_threshold_seq = last_checkpoint_ - threshold;
  LOG(INFO) << "Migrating data older than seq: " << cold_threshold_seq;
  
  auto all_items = storage_->GetAllItemsWithSeq();
  int migrated = 0;
  
  for (const auto& [key, value_list] : all_items) {
    for (const auto& [value, seq] : value_list) {
      if (seq >= cold_threshold_seq) {
        continue;
      }
      
      std::string cid = SerializeAndUpload(key, value);
      if (!cid.empty()) {
        if (UpdateIndex(key, cid)) {
          DeleteFromWarmStorage(key);
          migrated_keys_++;
          migrated++;
        }
      }
      
      if (migrated >= config_.batch_size()) {
        LOG(INFO) << "Batch size reached: " << migrated;
        break;
      }
    }
    
    if (migrated >= config_.batch_size()) {
      break;
    }
  }
  
  LOG(INFO) << "Migration complete: " << migrated << " keys migrated";
  return migrated > 0;
}

std::string MigrationDaemon::SerializeAndUpload(const std::string& key,
                                          const std::string& value) {
  std::string data = key + ":" + value;
  std::string cid = ipfs_client_->Add(data);
  
  if (cid.empty()) {
    LOG(ERROR) << "Failed to upload to IPFS: key=" << key;
    return "";
  }
  
  LOG(INFO) << "Uploaded to IPFS: key=" << key << ", cid=" << cid;
  return cid;
}

bool MigrationDaemon::UpdateIndex(const std::string& key,
                              const std::string& cid) {
  LOG(INFO) << "Updating index: key=" << key << ", cid=" << cid;
  return true;
}

bool MigrationDaemon::DeleteFromWarmStorage(const std::string& key) {
  LOG(INFO) << "Delete from warm storage: key=" << key;
  return true;
}

std::unique_ptr<MigrationDaemon> NewMigrationDaemon(
    std::unique_ptr<Storage> storage,
    const IPFSConfig& ipfs_config,
    const TieredStorageConfig& tiered_config) {
  auto ipfs_client = IPFSClient::Create(ipfs_config);
  return std::make_unique<MigrationDaemon>(
      std::move(storage),
      std::move(ipfs_client),
      tiered_config);
}

}  // namespace sidecar
}  // namespace storage
}  // namespace resdb