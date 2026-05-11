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
#include <memory>
#include <string>
#include <thread>

#include "chain/storage/interface/secondary_index.h"
#include "chain/storage/storage.h"
#include "chain/storage/ipfs_client.h"
#include "chain/storage/proto/ipfs_config.pb.h"
#include "chain/storage/proto/tiered_index.pb.h"

namespace resdb {
namespace storage {

class TieredStorage : public Storage {
 public:
  TieredStorage(std::unique_ptr<Storage> hot_storage,
                std::unique_ptr<Storage> warm_storage,
                std::unique_ptr<IPFSClient> cold_client,
                const TieredStorageConfig& config);
  ~TieredStorage() override;

  int SetValue(const std::string& key, const std::string& value) override;
  int SetValueWithSeq(const std::string& key, const std::string& value,
                     uint64_t seq) override;
  std::string GetValue(const std::string& key) override;
  std::pair<std::string, uint64_t> GetValueWithSeq(const std::string& key,
                                                uint64_t seq) override;
  std::string GetRange(const std::string& min_key,
                      const std::string& max_key) override;

  int SetValueWithVersion(const std::string& key, const std::string& value,
                         int version) override;
  std::pair<std::string, int> GetValueWithVersion(const std::string& key,
                                                int version) override;

  std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
  GetAllItemsWithSeq() override;
  std::map<std::string, std::pair<std::string, int>> GetAllItems() override;
  std::map<std::string, std::pair<std::string, int>> GetKeyRange(
      const std::string& min_key, const std::string& max_key) override;

  std::vector<std::pair<std::string, int>> GetHistory(const std::string& key,
                                                     int min_version,
                                                     int max_version) override;
  std::vector<std::pair<std::string, int>> GetTopHistory(const std::string& key,
                                                       int number) override;

  bool Flush() override;
  uint64_t GetLastCheckpoint() override;

  void SetColdThreshold(int checkpoint_threshold);
  int GetColdThreshold() const;
  bool IsColdData(const std::string& key) const;
  bool IsColdEnabled() const;
  std::string GetCID(const std::string& key) const;
  bool IsTiered() const { return config_.enabled(); }

  static std::unique_ptr<Storage> Create(
      std::unique_ptr<Storage> hot_storage,
      std::unique_ptr<Storage> warm_storage,
      const IPFSConfig& ipfs_config,
      const TieredStorageConfig& tiered_config);

  bool LoadManifest();
  bool SaveManifest();
  bool AddToIndex(const std::string& key, const std::string& cid,
                uint64_t min_seq, uint64_t max_seq);
  uint64_t GetColdKeyCount() const { return manifest_.cold_keys(); }

  void StartMigration();
  void StopMigration();
  uint64_t GetMigratedKeyCount() const { return migrated_keys_.load(); }

  bool LoadManifestFromStorage(Storage* storage);

 private:
  std::string GetValueWithFallback(const std::string& key);
  std::string GetValueFromCold(const std::string& key);
  std::string GetIndexCID(const std::string& key) const;

  int SetValueInternal(Storage* storage, const std::string& key,
                       const std::string& value);
  std::string GetValueInternal(Storage* storage, const std::string& key);
  bool SaveManifestToStorage(Storage* storage);

  void MigrationLoop();
  bool MigrateColdData();
  bool MigrateKey(const std::string& key, const std::string& value, uint64_t seq);
  bool DeleteKeyFromWarm(const std::string& key);
  bool DeleteKeyFromHot(const std::string& key);

  std::unique_ptr<Storage> hot_storage_;
  std::unique_ptr<Storage> warm_storage_;
  std::unique_ptr<IPFSClient> cold_client_;
  TieredStorageConfig config_;

  IndexManifest manifest_;
  std::string manifest_key_;
  bool manifest_loaded_ = false;

  std::unique_ptr<SecondaryIndex> index_;
  std::atomic<uint64_t> max_seq_{0};

  std::thread migration_thread_;
  std::atomic<bool> migration_running_{false};
  std::atomic<uint64_t> migrated_keys_{0};
  uint64_t last_migrated_seq_ = 0;
};

}  // namespace storage
}  // namespace resdb