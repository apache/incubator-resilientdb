/*
 * DEPRECATED: This file is no longer used.
 * Migration logic has been integrated directly into TieredStorage as a background thread.
 * The standalone migration daemon is obsolete because LevelDB does not support concurrent
 * access from multiple processes (LOCK file conflict).
 *
 * This file is preserved for reference only. Do not modify.
 *
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

#include <glog/logging.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <cstring>
#include <thread>

#include "chain/storage/leveldb.h"
#include "chain/storage/ipfs_client.h"
#include "chain/storage/proto/ipfs_config.pb.h"
#include "sidecar/migration_daemon/migration_daemon.h"

namespace {
std::string g_leveldb_path = "/tmp/nexres-leveldb";
std::string g_ipfs_endpoint = "http://localhost:5001";
int g_cold_threshold = 2;
int g_poll_interval = 60;
int g_batch_size = 1000;
bool g_enable_migration = true;
}

void ShowUsage() {
  std::cout << "Usage: migration_daemon [options]\n"
            << "\n"
            << "Options:\n"
            << "  --config <file>       Config file path (optional)\n"
            << "  --leveldb_path <path> LevelDB path (default: /tmp/nexres-leveldb)\n"
            << "  --ipfs_endpoint <url> IPFS API endpoint (default: http://localhost:5001)\n"
            << "  --cold_threshold <N>   Cold threshold in checkpoints (default: 2)\n"
            << "  --poll_interval <sec>  Poll interval in seconds (default: 60)\n"
            << "  --batch_size <N>       Migration batch size (default: 1000)\n"
            << "  --enable_migration   Enable migration (default: true)\n"
            << "  --help              Show this help\n"
            << std::endl;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  for (int i = 1; i < argc - 1; i += 2) {
    std::string arg = argv[i];
    if (arg == "--leveldb_path") {
      g_leveldb_path = argv[i + 1];
    } else if (arg == "--ipfs_endpoint") {
      g_ipfs_endpoint = argv[i + 1];
    } else if (arg == "--cold_threshold") {
      g_cold_threshold = std::stoi(argv[i + 1]);
    } else if (arg == "--poll_interval") {
      g_poll_interval = std::stoi(argv[i + 1]);
    } else if (arg == "--batch_size") {
      g_batch_size = std::stoi(argv[i + 1]);
    } else if (arg == "--enable_migration") {
      g_enable_migration = (std::string(argv[i + 1]) == "true");
    } else if (arg == "--help") {
      ShowUsage();
      return 0;
    }
  }
  
  LOG(INFO) << "Starting Migration Sidecar Daemon";
  LOG(INFO) << "LevelDB path: " << g_leveldb_path;
  LOG(INFO) << "IPFS endpoint: " << g_ipfs_endpoint;
  LOG(INFO) << "Cold threshold: " << g_cold_threshold;
  
  resdb::storage::LevelDBInfo leveldb_config;
  leveldb_config.set_path(g_leveldb_path);
  leveldb_config.set_write_buffer_size_mb(2);
  leveldb_config.set_write_batch_size(3);
  
  auto storage = resdb::storage::NewResLevelDB(
      leveldb_config.path(),
      std::make_optional(leveldb_config));
  
  if (!storage) {
    LOG(ERROR) << "Failed to create LevelDB storage";
    return 1;
  }
  
  resdb::storage::IPFSConfig ipfs_config;
  ipfs_config.set_api_endpoint(g_ipfs_endpoint);
  ipfs_config.set_enabled(g_enable_migration);
  ipfs_config.set_gateway_endpoint("http://localhost:8080");
  ipfs_config.set_timeout_ms(30000);
  ipfs_config.set_max_retries(3);
  
  resdb::storage::TieredStorageConfig tiered_config;
  tiered_config.set_cold_threshold_checkpoint(g_cold_threshold);
  tiered_config.set_enabled(g_enable_migration);
  tiered_config.set_poll_interval_seconds(g_poll_interval);
  tiered_config.set_batch_size(g_batch_size);
  tiered_config.set_auto_migration_enabled(g_enable_migration);
  
  auto daemon = resdb::storage::sidecar::NewMigrationDaemon(
      std::move(storage),
      ipfs_config,
      tiered_config);
  
  if (!daemon) {
    LOG(ERROR) << "Failed to create migration daemon";
    return 1;
  }
  
  daemon->Start();
  
  LOG(INFO) << "Migration daemon running. Press Ctrl+C to stop.";
  
  while (daemon->IsRunning()) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  
  LOG(INFO) << "Migration daemon stopped";
  return 0;
}
