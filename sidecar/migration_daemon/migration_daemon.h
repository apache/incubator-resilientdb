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

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "chain/storage/storage.h"
#include "chain/storage/ipfs_client.h"
#include "chain/storage/proto/ipfs_config.pb.h"
#include "chain/storage/proto/tiered_index.pb.h"

namespace resdb {
namespace storage {
namespace sidecar {

class MigrationDaemon {
 public:
  MigrationDaemon(std::unique_ptr<Storage> storage,
                 std::unique_ptr<IPFSClient> ipfs_client,
                 const TieredStorageConfig& config);
  
  ~MigrationDaemon();
  
  void Start();
  void Stop();
  void RunMigration();
  
  bool IsRunning() const { return running_.load(); }
  uint64_t GetMigratedKeyCount() const { return migrated_keys_; }
  uint64_t GetLastCheckpoint() const { return last_checkpoint_; }
  
 private:
  void PollLoop();
  bool MigrateDataOlderThanThreshold();
  std::string SerializeAndUpload(const std::string& key,
                                  const std::string& value);
  bool UpdateIndex(const std::string& key, const std::string& cid);
  bool DeleteFromWarmStorage(const std::string& key);
  
  std::unique_ptr<Storage> storage_;
  std::unique_ptr<IPFSClient> ipfs_client_;
  TieredStorageConfig config_;
  
  std::thread poll_thread_;
  std::atomic<bool> running_;
  std::atomic<uint64_t> migrated_keys_;
  uint64_t last_checkpoint_;
};

std::unique_ptr<MigrationDaemon> NewMigrationDaemon(
    std::unique_ptr<Storage> storage,
    const IPFSConfig& ipfs_config,
    const TieredStorageConfig& tiered_config);

}  // namespace sidecar
}  // namespace storage
}  // namespace resdb