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

#include "chain/storage/tiered_storage.h"

#include <chrono>
#include <google/protobuf/util/json_util.h>
#include <glog/logging.h>

#include "chain/storage/in_memory_index.h"
#include "chain/storage/leveldb.h"

namespace resdb {
namespace storage {

namespace {

constexpr const char* kManifestKey = "_tiered_manifest";
constexpr const char* kColdThresholdKey = "_cold_threshold";
constexpr int kDefaultColdThreshold = 2;
constexpr int kDefaultPollIntervalSeconds = 60;
constexpr int kDefaultBatchSize = 100;
constexpr int kDefaultCheckpointWatermark = 5;

}

TieredStorage::TieredStorage(
    std::unique_ptr<Storage> hot_storage,
    std::unique_ptr<Storage> warm_storage,
    std::unique_ptr<IPFSClient> cold_client,
    const TieredStorageConfig& config)
    : hot_storage_(std::move(hot_storage)),
      warm_storage_(std::move(warm_storage)),
      cold_client_(std::move(cold_client)),
      config_(config),
      manifest_key_(kManifestKey) {
  if (!config_.enabled()) {
    LOG(WARNING) << "TieredStorage created but tiering is disabled";
  } else {
    LOG(INFO) << "TieredStorage enabled with cold_threshold = "
              << config_.cold_threshold_checkpoint();
  }

  if (config_.cold_threshold_checkpoint() < 0) {
    config_.set_cold_threshold_checkpoint(kDefaultColdThreshold);
  }

  if (config_.checkpoint_watermark() <= 0) {
    config_.set_checkpoint_watermark(kDefaultCheckpointWatermark);
  }

  if (config_.enabled() && cold_client_ && cold_client_->IsEnabled()) {
    index_ = std::make_unique<InMemoryHashIndex>();
    LoadManifestFromStorage(warm_storage_.get());
    StartMigration();
  }
}

TieredStorage::~TieredStorage() {
  StopMigration();
}

int TieredStorage::SetValue(const std::string& key, const std::string& value) {
  return SetValueInternal(hot_storage_.get(), key, value);
}

int TieredStorage::SetValueWithSeq(const std::string& key,
                                   const std::string& value, uint64_t seq) {
  int ret = hot_storage_->SetValueWithSeq(key, value, seq);
  if (ret == 0) {
    uint64_t prev = max_seq_.load();
    while (seq > prev && !max_seq_.compare_exchange_weak(prev, seq)) {
    }
  }
  return ret;
}

std::string TieredStorage::GetValue(const std::string& key) {
  return GetValueWithFallback(key);
}

std::pair<std::string, uint64_t> TieredStorage::GetValueWithSeq(
    const std::string& key, uint64_t seq) {
  auto hot_result = hot_storage_->GetValueWithSeq(key, seq);
  if (!hot_result.first.empty()) {
    LOG(INFO) << "TieredStorage: found value in hot storage for key: " << key;
    return hot_result;
  }

  auto warm_result = warm_storage_->GetValueWithSeq(key, seq);
  if (!warm_result.first.empty()) {
    LOG(INFO) << "TieredStorage: found value in warm storage for key: " << key;
    return warm_result;
  }

  if (IsColdEnabled()) {
    std::string cold_value = GetValueFromCold(key);
    if (!cold_value.empty()) {
      LOG(INFO) << "TieredStorage: found value in cold storage for key: " << key;
      return {cold_value, 0};
    }
  }

  LOG(INFO) << "TieredStorage: key not found: " << key;
  return {"", 0};
}

std::string TieredStorage::GetRange(const std::string& min_key,
                                  const std::string& max_key) {
  std::string result = warm_storage_->GetRange(min_key, max_key);
  if (!result.empty()) {
    return result;
  }

  return result;
}

int TieredStorage::SetValueWithVersion(const std::string& key,
                                    const std::string& value, int version) {
  return hot_storage_->SetValueWithVersion(key, value, version);
}

std::pair<std::string, int> TieredStorage::GetValueWithVersion(
    const std::string& key, int version) {
  auto result = hot_storage_->GetValueWithVersion(key, version);
  if (!result.first.empty()) {
    return result;
  }

  return warm_storage_->GetValueWithVersion(key, version);
}

std::map<std::string, std::vector<std::pair<std::string, uint64_t>>>
TieredStorage::GetAllItemsWithSeq() {
  return warm_storage_->GetAllItemsWithSeq();
}

std::map<std::string, std::pair<std::string, int>> TieredStorage::GetAllItems() {
  return warm_storage_->GetAllItems();
}

std::map<std::string, std::pair<std::string, int>> TieredStorage::GetKeyRange(
    const std::string& min_key, const std::string& max_key) {
  return warm_storage_->GetKeyRange(min_key, max_key);
}

std::vector<std::pair<std::string, int>> TieredStorage::GetHistory(
    const std::string& key, int min_version, int max_version) {
  auto result = hot_storage_->GetHistory(key, min_version, max_version);
  if (!result.empty()) {
    return result;
  }

  return warm_storage_->GetHistory(key, min_version, max_version);
}

std::vector<std::pair<std::string, int>> TieredStorage::GetTopHistory(
    const std::string& key, int number) {
  auto result = hot_storage_->GetTopHistory(key, number);
  if (!result.empty()) {
    return result;
  }

  return warm_storage_->GetTopHistory(key, number);
}

bool TieredStorage::Flush() {
  hot_storage_->Flush();
  warm_storage_->Flush();
  return true;
}

uint64_t TieredStorage::GetLastCheckpoint() {
  if (config_.hot_backend() == TieredStorageConfig::LEVELDB && hot_storage_) {
    return hot_storage_->GetLastCheckpoint();
  }
  return max_seq_.load();
}

void TieredStorage::SetColdThreshold(int checkpoint_threshold) {
  config_.set_cold_threshold_checkpoint(checkpoint_threshold);
  LOG(INFO) << "Set cold threshold to " << checkpoint_threshold;
}

int TieredStorage::GetColdThreshold() const {
  return config_.cold_threshold_checkpoint();
}

bool TieredStorage::IsColdData(const std::string& key) const {
  return !GetIndexCID(key).empty();
}

bool TieredStorage::IsColdEnabled() const {
  return config_.enabled() && cold_client_ && cold_client_->IsEnabled();
}

std::string TieredStorage::GetCID(const std::string& key) const {
  return GetIndexCID(key);
}

std::unique_ptr<Storage> TieredStorage::Create(
    std::unique_ptr<Storage> hot_storage,
    std::unique_ptr<Storage> warm_storage,
    const IPFSConfig& ipfs_config,
    const TieredStorageConfig& tiered_config) {
  if (!tiered_config.enabled()) {
    LOG(INFO) << "Tiered storage disabled, using warm storage only";
    return warm_storage;
  }

  auto cold_client = IPFSClient::Create(ipfs_config);
  if (!cold_client->IsEnabled()) {
    LOG(WARNING) << "IPFS client not enabled, falling back to warm storage";
    return warm_storage;
  }

  return std::make_unique<TieredStorage>(
      std::move(hot_storage),
      std::move(warm_storage),
      std::move(cold_client),
      tiered_config);
}

std::string TieredStorage::GetValueWithFallback(const std::string& key) {
  std::string value = GetValueInternal(hot_storage_.get(), key);
  if (!value.empty()) {
    LOG(INFO) << "TieredStorage: found value in hot storage for key: " << key;
    return value;
  }

  value = GetValueInternal(warm_storage_.get(), key);
  if (!value.empty()) {
    LOG(INFO) << "TieredStorage: found value in warm storage for key: " << key;
    return value;
  }

  if (IsColdEnabled()) {
    value = GetValueFromCold(key);
    if (!value.empty()) {
      LOG(INFO) << "TieredStorage: found value in cold storage for key: " << key;
      return value;
    }
  }

  LOG(INFO) << "TieredStorage: key not found: " << key;
  return "";
}

std::string TieredStorage::GetValueFromCold(const std::string& key) {
  std::string cid = GetIndexCID(key);
  if (cid.empty()) {
    LOG(INFO) << "TieredStorage: no CID found for key: " << key;
    return "";
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::string result = cold_client_->Cat(cid);
  auto end = std::chrono::high_resolution_clock::now();
  auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  LOG(INFO) << "[timer] cold_read key=" << key << " cid=" << cid << " us=" << us;
  return result;
}

std::string TieredStorage::GetIndexCID(const std::string& key) const {
  if (index_) {
    auto start = std::chrono::high_resolution_clock::now();
    std::string cid = index_->Get(key);
    auto end = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    LOG(INFO) << "[timer] index_lookup key=" << key << " us=" << us;
    return cid;
  }
  return "";
}

int TieredStorage::SetValueInternal(Storage* storage,
                                  const std::string& key,
                                  const std::string& value) {
  if (!storage) {
    LOG(ERROR) << "TieredStorage: storage is null";
    return -1;
  }
  return storage->SetValue(key, value);
}

std::string TieredStorage::GetValueInternal(Storage* storage,
                                             const std::string& key) {
  if (!storage) {
    LOG(ERROR) << "TieredStorage: storage is null";
    return "";
  }
  return storage->GetValue(key);
}

bool TieredStorage::LoadManifest() {
  return LoadManifestFromStorage(warm_storage_.get());
}

bool TieredStorage::SaveManifest() {
  return SaveManifestToStorage(warm_storage_.get());
}

bool TieredStorage::AddToIndex(const std::string& key, const std::string& cid,
                          uint64_t min_seq, uint64_t max_seq) {
  if (!IsColdEnabled() || !index_) {
    LOG(WARNING) << "TieredStorage: cold storage not enabled or no index";
    return false;
  }

  index_->Add(key, cid);

  auto* mapping = manifest_.add_range_mappings();
  mapping->set_start_key(key);
  mapping->set_end_key(key);
  mapping->set_ipfs_cid(cid);
  mapping->set_min_checkpoint(min_seq);
  mapping->set_max_checkpoint(max_seq);

  manifest_.set_total_keys(manifest_.total_keys() + 1);
  manifest_.set_cold_keys(manifest_.cold_keys() + 1);
  manifest_.set_last_updated_timestamp(time(nullptr));

  LOG(INFO) << "TieredStorage: added to index, key=" << key
            << ", cid=" << cid << ", range=[" << min_seq << "," << max_seq << "]";

  return SaveManifestToStorage(warm_storage_.get());
}

bool TieredStorage::LoadManifestFromStorage(Storage* storage) {
  if (!storage) {
    LOG(ERROR) << "TieredStorage: cannot load manifest, storage is null";
    return false;
  }

  std::string manifest_data = storage->GetValue(manifest_key_);
  if (manifest_data.empty()) {
    LOG(INFO) << "TieredStorage: no manifest found, starting fresh";
    manifest_.set_version(1);
    manifest_.set_total_keys(0);
    manifest_.set_cold_keys(0);
    manifest_.set_last_updated_timestamp(time(nullptr));
    return true;
  }

  if (!manifest_.ParseFromString(manifest_data)) {
    LOG(ERROR) << "TieredStorage: failed to parse manifest";
    return false;
  }

  if (index_) {
    index_->Clear();
    for (const auto& mapping : manifest_.range_mappings()) {
      index_->Add(mapping.start_key(), mapping.ipfs_cid());
    }
  }

  LOG(INFO) << "TieredStorage: loaded manifest, "
            << "total_keys=" << manifest_.total_keys()
            << ", cold_keys=" << manifest_.cold_keys();
  manifest_loaded_ = true;
  return true;
}

bool TieredStorage::SaveManifestToStorage(Storage* storage) {
  if (!storage) {
    LOG(ERROR) << "TieredStorage: cannot save manifest, storage is null";
    return false;
  }

  std::string manifest_data;
  if (!manifest_.SerializeToString(&manifest_data)) {
    LOG(ERROR) << "TieredStorage: failed to serialize manifest";
    return false;
  }

  int ret = storage->SetValue(manifest_key_, manifest_data);
  if (ret != 0) {
    LOG(ERROR) << "TieredStorage: failed to save manifest, ret=" << ret;
    return false;
  }

  LOG(INFO) << "TieredStorage: saved manifest, "
            << "size=" << manifest_data.size() << " bytes";
  return true;
}

void TieredStorage::StartMigration() {
  if (!IsColdEnabled()) {
    LOG(WARNING) << "TieredStorage: cannot start migration, cold storage not enabled";
    return;
  }

  if (migration_running_.load()) {
    LOG(WARNING) << "TieredStorage: migration thread already running";
    return;
  }

  if (!config_.auto_migration_enabled()) {
    LOG(INFO) << "TieredStorage: auto migration not enabled in config";
    return;
  }

  migration_running_.store(true);
  migration_thread_ = std::thread(&TieredStorage::MigrationLoop, this);
  LOG(INFO) << "TieredStorage: migration thread started";
}

void TieredStorage::StopMigration() {
  if (!migration_running_.load()) {
    return;
  }

  migration_running_.store(false);
  if (migration_thread_.joinable()) {
    migration_thread_.join();
  }
  LOG(INFO) << "TieredStorage: migration thread stopped, "
            << "total_migrated=" << migrated_keys_.load();
}

void TieredStorage::MigrationLoop() {
  int poll_interval = config_.poll_interval_seconds();
  if (poll_interval <= 0) {
    poll_interval = kDefaultPollIntervalSeconds;
  }

  LOG(INFO) << "TieredStorage: migration polling started, interval="
            << poll_interval << "s";

  while (migration_running_.load()) {
    try {
      MigrateColdData();
    } catch (const std::exception& e) {
      LOG(ERROR) << "TieredStorage: migration error: " << e.what();
    }

    for (int i = 0; i < poll_interval && migration_running_.load(); ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  LOG(INFO) << "TieredStorage: migration polling stopped";
}

bool TieredStorage::MigrateColdData() {
  auto cycle_start = std::chrono::high_resolution_clock::now();
  uint64_t current_checkpoint = GetLastCheckpoint();
  int watermark = config_.checkpoint_watermark();
  if (watermark <= 0) {
    watermark = kDefaultCheckpointWatermark;
  }
  uint64_t threshold = config_.cold_threshold_checkpoint() * watermark;

  if (current_checkpoint <= threshold) {
    LOG(INFO) << "TieredStorage: checkpoint " << current_checkpoint
              << " not past threshold " << threshold;
    return false;
  }

  uint64_t cold_threshold_seq = current_checkpoint - threshold;

  LOG(INFO) << "TieredStorage: migrating data with seq < " << cold_threshold_seq;

  int batch_size = config_.batch_size();
  if (batch_size <= 0) {
    batch_size = kDefaultBatchSize;
  }

  int migrated = 0;

  auto all_items = hot_storage_->GetAllItemsWithSeq();

  for (const auto& [key, value_list] : all_items) {
    for (const auto& [value, seq] : value_list) {
      if (seq > cold_threshold_seq) {
        continue;
      }

      if (MigrateKey(key, value, seq)) {
        migrated++;
      }

      if (migrated >= batch_size) {
        LOG(INFO) << "TieredStorage: batch size reached: " << migrated;
        break;
      }
    }

    if (migrated >= batch_size) {
      break;
    }
  }

  auto cycle_end = std::chrono::high_resolution_clock::now();
  auto cycle_us = std::chrono::duration_cast<std::chrono::microseconds>(cycle_end - cycle_start).count();
  LOG(INFO) << "[timer] migrate_cycle keys=" << migrated << " us=" << cycle_us;

  return migrated > 0;
}

bool TieredStorage::MigrateKey(const std::string& key, const std::string& value, uint64_t seq) {
  std::string old_cid = GetIndexCID(key);

  auto add_start = std::chrono::high_resolution_clock::now();
  std::string cid = cold_client_->Add(value);
  auto add_end = std::chrono::high_resolution_clock::now();
  auto add_us = std::chrono::duration_cast<std::chrono::microseconds>(add_end - add_start).count();
  LOG(INFO) << "[timer] ipfs_add key=" << key << " us=" << add_us;
  if (cid.empty()) {
    LOG(ERROR) << "TieredStorage: failed to upload to IPFS: key=" << key;
    return false;
  }

  LOG(INFO) << "TieredStorage: uploaded to IPFS: key=" << key << ", cid=" << cid;

  if (!AddToIndex(key, cid, seq, seq)) {
    LOG(ERROR) << "TieredStorage: failed to add to index: key=" << key;
    return false;
  }

  if (DeleteKeyFromHot(key)) {
    LOG(INFO) << "TieredStorage: evicted from hot storage: key=" << key;
  }

  if (!old_cid.empty() && old_cid != cid) {
    if (cold_client_->Unpin(old_cid)) {
      LOG(INFO) << "TieredStorage: unpinned stale CID=" << old_cid
                << " for key=" << key;
    } else {
      LOG(WARNING) << "TieredStorage: failed to unpin stale CID=" << old_cid
                   << " for key=" << key;
    }
  }

  migrated_keys_.fetch_add(1);
  LOG(INFO) << "TieredStorage: key migrated successfully: key=" << key;
  return true;
}

bool TieredStorage::DeleteKeyFromWarm(const std::string& key) {
  if (!IsTiered() || !IsColdEnabled()) {
    LOG(ERROR) << "TieredStorage: delete attempted but tiered storage not enabled";
    return false;
  }

  auto* deletable = dynamic_cast<DeletableStorage*>(warm_storage_.get());
  if (!deletable) {
    LOG(ERROR) << "TieredStorage: warm storage does not support deletion";
    return false;
  }

  return deletable->DeleteKey(key);
}

bool TieredStorage::DeleteKeyFromHot(const std::string& key) {
  auto* deletable = dynamic_cast<DeletableStorage*>(hot_storage_.get());
  if (!deletable) {
    LOG(WARNING) << "TieredStorage: hot storage does not support deletion";
    return false;
  }
  return deletable->DeleteKey(key);
}

}  // namespace storage
}  // namespace resdb