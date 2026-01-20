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
*
*/

/**
 * Static data obtained from running the bazel command on the server.
 * TODO: find a way to get the data from the server by running the command directly.
 */
export const dots2 = [
    [
        `
        digraph {
        node [shape = box, style=filled, width=4, height = 1, fontname="Rethink Sans,Arial,sans-serif"];  // White text
        edge [color=white];  // White arrows
        bgcolor=transparent;  // Transparent background
  
        // KV Nodes - Grouped together and colored lightblue
        "//service/kv:kv_service" [label="//service/kv:kv_service", fillcolor="#80C7FF"];
        "//executor/kv:kv_executor" [label="//executor/kv:kv_executor", fillcolor="#80C7FF"];
        "//proto/kv:kv_cc_proto" [label="//proto/kv:kv_cc_proto", fillcolor="#80C7FF"];
        "//proto/kv:kv_proto" [label="//proto/kv:kv_proto", fillcolor="#80C7FF"];
  
        // Storage Nodes - Grouped together and colored lightgreen
        "//chain/storage:memory_db" [label="//chain/storage:memory_db", fillcolor="#A1E2A1"];
        "//chain/storage:leveldb" [label="//chain/storage:leveldb", fillcolor="#A1E2A1"];
        "//chain/storage:lmdb" [label="//chain/storage:lmdb", fillcolor="#A1E2A1"];
        "//chain/storage:storage" [label="//chain/storage:storage", fillcolor="#A1E2A1"];
  
        // Service Nodes - Grouped together and colored lightcoral
        "//service/kv:kv_service" [label="//service/kv:kv_service", fillcolor="#FFB6B6"];
        "//service/utils:server_factory" [label="//service/utils:server_factory", fillcolor="#FFB6B6"];
  
        // Platform Nodes - Grouped together and colored lightyellow
        "//platform/config:resdb_config_utils" [label="//platform/config:resdb_config_utils", fillcolor="#FFEB99"];
  
        // Common Nodes - Grouped together and colored lightgray
        "//common:comm" [label="//common:comm", fillcolor="#D3D3D3"];
  
        // Defining edges
        "//service/kv:kv_service" -> "//chain/storage:setting:enable_leveldb_setting";
        "//service/kv:kv_service" -> "//platform/config:resdb_config_utils";
        "//service/kv:kv_service" -> "//executor/kv:kv_executor";
        "//service/kv:kv_service" -> "//service/utils:server_factory";
        "//service/kv:kv_service" -> "//common:comm";
        "//service/kv:kv_service" -> "//proto/kv:kv_cc_proto";
        "//service/kv:kv_service" -> "//chain/storage:memory_db";
        "//service/kv:kv_service" -> "//chain/storage:leveldb";
        "//service/kv:kv_service" -> "//chain/storage:lmdb";
  
        "//chain/storage:lmdb" -> "//chain/storage:storage";
        "//chain/storage:lmdb" -> "//proto/kv:kv_cc_proto";
        "//chain/storage:lmdb" -> "//common:comm";
  
        "//chain/storage:leveldb" -> "//chain/storage:storage";
        "//chain/storage:leveldb" -> "//proto/kv:kv_cc_proto";
        "//chain/storage:leveldb" -> "//common:comm";
  
        "//proto/kv:kv_cc_proto" -> "//proto/kv:kv_proto";
  
        "//service/utils:server_factory" -> "//platform/config:resdb_config_utils";
        "//service/utils:server_factory" -> "//common:comm";
  
        "//executor/kv:kv_executor" -> "//chain/storage:storage";
        "//executor/kv:kv_executor" -> "//common:comm";
        "//executor/kv:kv_executor" -> "//proto/kv:kv_cc_proto";
  
        "//platform/config:resdb_config_utils" -> "//common:comm";
        "//common:comm" -> "//chain/storage:storage";
        }
      `,
    ],
];
export const dots3 = [
    [
        `
        digraph mygraph {
  node [shape=box];
  "//service/kv:kv_service"
  "//service/kv:kv_service" -> "//service/kv:kv_service.cpp\n//chain/storage/setting:enable_leveldb_setting"
  "//service/kv:kv_service" -> "//platform/config:resdb_config_utils"
  "//service/kv:kv_service" -> "//executor/kv:kv_executor"
  "//service/kv:kv_service" -> "//service/utils:server_factory"
  "//service/kv:kv_service" -> "//common:comm"
  "//service/kv:kv_service" -> "//proto/kv:kv_cc_proto"
  "//service/kv:kv_service" -> "//chain/storage:memory_db"
  "//service/kv:kv_service" -> "//chain/storage:leveldb"
  [label="//chain/storage/setting:enable_leveldb_setting"];
  "//service/kv:kv_service" -> "//chain/storage:lmdb"
  "//chain/storage:leveldb"
  "//chain/storage:leveldb" -> "//chain/storage:leveldb.cpp\n//chain/storage:leveldb.h"
  "//chain/storage:leveldb" -> "//chain/storage:storage"
  "//chain/storage:leveldb" -> "//chain/storage/proto:kv_cc_proto"
  "//chain/storage:leveldb" -> "//chain/storage/proto:leveldb_config_cc_proto"
  "//chain/storage:leveldb" -> "//common:comm"
  "//chain/storage:leveldb" -> "//third_party:leveldb"
  "//third_party:leveldb"
  "//third_party:leveldb" -> "@com_google_leveldb//:leveldb"
  "@com_google_leveldb//:leveldb"
  "//chain/storage:leveldb.cpp\n//chain/storage:leveldb.h"
  "//chain/storage:memory_db"
  "//chain/storage:memory_db" -> "//chain/storage:memory_db.cpp\n//chain/storage:memory_db.h"
  "//chain/storage:memory_db" -> "//chain/storage:storage"
  "//chain/storage:memory_db" -> "//common:comm"
  "//service/utils:server_factory"
  "//service/utils:server_factory" -> "//service/utils:server_factory.cpp\n//service/utils:server_factory.h"
  "//service/utils:server_factory" -> "//platform/config:resdb_config_utils"
  "//service/utils:server_factory" -> "//platform/consensus/ordering/pbft:consensus_manager_pbft"
  "//service/utils:server_factory" -> "//platform/networkstrate:service_network"
  "//platform/networkstrate:service_network"
  "//platform/networkstrate:service_network" -> "//platform/common/data_comm:network_comm\n//platform/networkstrate:service_network.cpp\n//platform/networkstrate:service_network.h\n//platform/statistic:stats\n//platform/networkstrate:async_acceptor\n//platform/rdbc:acceptor\n//platform/networkstrate:service_interface\n//platform/common/queue:lock_free_queue\n//platform/proto:broadcast_cc_proto\n//platform/common/network:tcp_socket\n//platform/common/data_comm:data_comm"
  "//platform/common/data_comm:network_comm\n//platform/networkstrate:service_network.cpp\n//platform/networkstrate:service_network.h\n//platform/statistic:stats\n//platform/networkstrate:async_acceptor\n//platform/rdbc:acceptor\n//platform/networkstrate:service_interface\n//platform/common/queue:lock_free_queue\n//platform/proto:broadcast_cc_proto\n//platform/common/network:tcp_socket\n//platform/common/data_comm:data_comm"
  "//service/utils:server_factory.cpp\n//service/utils:server_factory.h"
  "//service/kv:kv_service.cpp\n//chain/storage/setting:enable_leveldb_setting"
  "//chain/storage:lmdb"
  "//chain/storage:lmdb" -> "//chain/storage:lmdb.h\n//chain/storage:lmdb.cpp"
  "//chain/storage:lmdb" -> "//chain/storage:storage"
  "//chain/storage:lmdb" -> "//chain/storage/proto:kv_cc_proto"
  "//chain/storage:lmdb" -> "//common:comm"
  "//chain/storage:lmdb.h\n//chain/storage:lmdb.cpp"
  "//chain/storage/proto:kv_cc_proto"
  "//chain/storage/proto:kv_cc_proto" -> "//chain/storage/proto:kv_proto"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:viewchange_manager\n//common/crypto:signature_verifier\n//platform/consensus/ordering/pbft:query\n//platform/consensus/ordering/pbft:performance_manager\n//platform/consensus/ordering/pbft:consensus_manager_pbft.h\n//platform/consensus/recovery:recovery\n//platform/consensus/ordering/pbft:checkpoint_manager\n//platform/networkstrate:consensus_manager\n//platform/consensus/ordering/pbft:commitment\n...and 2 more items"
  "//platform/consensus/ordering/pbft:viewchange_manager\n//common/crypto:signature_verifier\n//platform/consensus/ordering/pbft:query\n//platform/consensus/ordering/pbft:performance_manager\n//platform/consensus/ordering/pbft:consensus_manager_pbft.h\n//platform/consensus/recovery:recovery\n//platform/consensus/ordering/pbft:checkpoint_manager\n//platform/networkstrate:consensus_manager\n//platform/consensus/ordering/pbft:commitment\n...and 2 more items"
  "//chain/storage:memory_db.cpp\n//chain/storage:memory_db.h"
  "//chain/storage/proto:kv_proto"
  "//executor/kv:kv_executor"
  "//executor/kv:kv_executor" -> "//executor/kv:kv_executor.h\n//executor/kv:kv_executor.cpp"
  "//executor/kv:kv_executor" -> "//chain/storage:storage"
  "//executor/kv:kv_executor" -> "//common:comm"
  "//executor/kv:kv_executor" -> "//executor/common:transaction_manager"
  "//executor/kv:kv_executor" -> "//platform/config:resdb_config_utils"
  "//executor/kv:kv_executor" -> "//proto/kv:kv_cc_proto"
  "//proto/kv:kv_cc_proto"
  "//proto/kv:kv_cc_proto" -> "//proto/kv:kv_proto"
  "//proto/kv:kv_proto"
  "//proto/kv:kv_proto" -> "//proto/kv:kv.proto"
  "//executor/common:transaction_manager"
  "//executor/common:transaction_manager" -> "//executor/common:transaction_manager.h\n//platform/proto:resdb_cc_proto\n//executor/common:transaction_manager.cpp"
  "//executor/common:transaction_manager" -> "//chain/storage:storage"
  "//executor/common:transaction_manager" -> "//common:comm"
  "//executor/common:transaction_manager.h\n//platform/proto:resdb_cc_proto\n//executor/common:transaction_manager.cpp"
  "//chain/storage:storage"
  "//chain/storage:storage" -> "//chain/storage:storage.h"
  "//chain/storage:storage.h"
  "//executor/kv:kv_executor.h\n//executor/kv:kv_executor.cpp"
  "//chain/storage/proto:leveldb_config_cc_proto"
  "//chain/storage/proto:leveldb_config_cc_proto" -> "//chain/storage/proto:leveldb_config_proto"
  "//chain/storage/proto:leveldb_config_proto"
  "//platform/config:resdb_config_utils"
  "//platform/config:resdb_config_utils" -> "//platform/config:resdb_config_utils.h\n//platform/config:resdb_config_utils.cpp"
  "//platform/config:resdb_config_utils" -> "//platform/config:resdb_config"
  "//platform/config:resdb_config"
  "//platform/config:resdb_config" -> "//platform/proto:replica_info_cc_proto\n//platform/config:resdb_config.cpp\n//platform/config:resdb_config.h"
  "//platform/config:resdb_config" -> "//common:comm"
  "//common:comm"
  "//common:comm" -> "//common:absl"
  "//common:comm" -> "//common:glog"
  "//common:absl"
  "//common:absl" -> "@com_google_absl//absl/status:status\n@com_google_absl//absl/status:statusor"
  "@com_google_absl//absl/status:status\n@com_google_absl//absl/status:statusor"
  "//platform/proto:replica_info_cc_proto\n//platform/config:resdb_config.cpp\n//platform/config:resdb_config.h"
  "//proto/kv:kv.proto"
  "//platform/config:resdb_config_utils.h\n//platform/config:resdb_config_utils.cpp"
  "//common:glog"
  "//common:glog" -> "@com_github_google_glog//:glog"
  "@com_github_google_glog//:glog"
}

        `
    ],
];
export const dots4 = [
    [
        `
        digraph mygraph {
  node [shape=box];
  "//service/kv:kv_service"
  "//service/kv:kv_service" -> "//chain/storage/setting:enable_leveldb_setting\n//service/kv:kv_service.cpp"
  "//service/kv:kv_service" -> "//platform/config:resdb_config_utils"
  "//service/kv:kv_service" -> "//executor/kv:kv_executor"
  "//service/kv:kv_service" -> "//service/utils:server_factory"
  "//service/kv:kv_service" -> "//common:comm"
  "//service/kv:kv_service" -> "//proto/kv:kv_cc_proto"
  "//service/kv:kv_service" -> "//chain/storage:memory_db"
  "//service/kv:kv_service" -> "//chain/storage:leveldb"
  [label="//chain/storage/setting:enable_leveldb_setting"];
  "//service/kv:kv_service" -> "//chain/storage:lmdb"
  "//service/utils:server_factory"
  "//service/utils:server_factory" -> "//service/utils:server_factory.cpp\n//service/utils:server_factory.h"
  "//service/utils:server_factory" -> "//platform/config:resdb_config_utils"
  "//service/utils:server_factory" -> "//platform/consensus/ordering/pbft:consensus_manager_pbft"
  "//service/utils:server_factory" -> "//platform/networkstrate:service_network"
  "//chain/storage/setting:enable_leveldb_setting\n//service/kv:kv_service.cpp"
  "//platform/networkstrate:service_network"
  "//platform/networkstrate:service_network" -> "//platform/networkstrate:service_network.cpp\n//platform/networkstrate:service_network.h"
  "//platform/networkstrate:service_network" -> "//platform/networkstrate:async_acceptor"
  "//platform/networkstrate:service_network" -> "//platform/networkstrate:service_interface"
  "//platform/networkstrate:service_network" -> "//platform/common/data_comm:data_comm"
  "//platform/networkstrate:service_network" -> "//platform/common/data_comm:network_comm"
  "//platform/networkstrate:service_network" -> "//platform/common/network:tcp_socket"
  "//platform/networkstrate:service_network" -> "//platform/common/queue:lock_free_queue"
  "//platform/networkstrate:service_network" -> "//platform/proto:broadcast_cc_proto"
  "//platform/networkstrate:service_network" -> "//platform/rdbc:acceptor"
  "//platform/networkstrate:service_network" -> "//platform/statistic:stats"
  "//platform/rdbc:acceptor"
  "//platform/rdbc:acceptor" -> "//platform/rdbc:acceptor.h\n//platform/rdbc:acceptor.cpp"
  "//platform/rdbc:acceptor" -> "//platform/common/data_comm:data_comm"
  "//platform/rdbc:acceptor" -> "//platform/common/data_comm:network_comm"
  "//platform/rdbc:acceptor" -> "//platform/common/network:tcp_socket"
  "//platform/rdbc:acceptor" -> "//platform/common/queue:lock_free_queue"
  "//platform/rdbc:acceptor" -> "//platform/networkstrate:server_comm"
  "//platform/rdbc:acceptor" -> "//platform/statistic:stats"
  "//platform/rdbc:acceptor.h\n//platform/rdbc:acceptor.cpp"
  "//platform/common/queue:lock_free_queue"
  "//platform/common/queue:lock_free_queue" -> "//common:boost_lockfree\n//platform/common/queue:lock_free_queue.h"
  "//platform/common/queue:lock_free_queue" -> "//common:comm"
  "//platform/networkstrate:async_acceptor"
  "//platform/networkstrate:async_acceptor" -> "//platform/networkstrate:async_acceptor.h\n//platform/networkstrate:async_acceptor.cpp"
  "//platform/networkstrate:async_acceptor" -> "//common:asio"
  "//platform/networkstrate:async_acceptor" -> "//common:comm"
  "//platform/networkstrate:async_acceptor" -> "//platform/config:resdb_config"
  "//platform/networkstrate:async_acceptor.h\n//platform/networkstrate:async_acceptor.cpp"
  "//platform/networkstrate:service_network.cpp\n//platform/networkstrate:service_network.h"
  "//chain/storage:leveldb"
  "//chain/storage:leveldb" -> "//chain/storage:leveldb.cpp\n//chain/storage:leveldb.h"
  "//chain/storage:leveldb" -> "//chain/storage:storage"
  "//chain/storage:leveldb" -> "//chain/storage/proto:kv_cc_proto"
  "//chain/storage:leveldb" -> "//chain/storage/proto:leveldb_config_cc_proto"
  "//chain/storage:leveldb" -> "//common:comm"
  "//chain/storage:leveldb" -> "//third_party:leveldb"
  "//chain/storage:leveldb.cpp\n//chain/storage:leveldb.h"
  "//service/utils:server_factory.cpp\n//service/utils:server_factory.h"
  "//third_party:leveldb"
  "//third_party:leveldb" -> "@com_google_leveldb//:leveldb"
  "@com_google_leveldb//:leveldb"
  "@com_google_leveldb//:leveldb" -> "@com_google_leveldb//:table/filter_block.h\n@com_google_leveldb//:include/leveldb/iterator.h\n@com_google_leveldb//:port/port_example.h\n@com_google_leveldb//:db/log_reader.cc\n@com_google_leveldb//:table/two_level_iterator.h\n@com_google_leveldb//:util/coding.cc\n@com_google_leveldb//:db/version_edit.cc\n@com_google_leveldb//:db/filename.cc\n@com_google_leveldb//:helpers/memenv/memenv.cc\n@com_google_leveldb//:util/hash.cc\n@com_google_leveldb//:include/leveldb/filter_policy.h\n...and 83 more items"
  "@com_google_leveldb//:table/filter_block.h\n@com_google_leveldb//:include/leveldb/iterator.h\n@com_google_leveldb//:port/port_example.h\n@com_google_leveldb//:db/log_reader.cc\n@com_google_leveldb//:table/two_level_iterator.h\n@com_google_leveldb//:util/coding.cc\n@com_google_leveldb//:db/version_edit.cc\n@com_google_leveldb//:db/filename.cc\n@com_google_leveldb//:helpers/memenv/memenv.cc\n@com_google_leveldb//:util/hash.cc\n@com_google_leveldb//:include/leveldb/filter_policy.h\n...and 83 more items"
  "//platform/common/data_comm:network_comm"
  "//platform/common/data_comm:network_comm" -> "//platform/common/data_comm:network_comm.h"
  "//platform/common/data_comm:network_comm" -> "//platform/common/data_comm:data_comm"
  "//platform/common/data_comm:network_comm" -> "//platform/common/network:tcp_socket"
  "//executor/kv:kv_executor"
  "//executor/kv:kv_executor" -> "//executor/kv:kv_executor.cpp\n//executor/kv:kv_executor.h"
  "//executor/kv:kv_executor" -> "//chain/storage:storage"
  "//executor/kv:kv_executor" -> "//common:comm"
  "//executor/kv:kv_executor" -> "//executor/common:transaction_manager"
  "//executor/kv:kv_executor" -> "//platform/config:resdb_config_utils"
  "//executor/kv:kv_executor" -> "//proto/kv:kv_cc_proto"
  "//platform/config:resdb_config_utils"
  "//platform/config:resdb_config_utils" -> "//platform/config:resdb_config_utils.cpp\n//platform/config:resdb_config_utils.h"
  "//platform/config:resdb_config_utils" -> "//platform/config:resdb_config"
  "//platform/config:resdb_config_utils.cpp\n//platform/config:resdb_config_utils.h"
  "//platform/common/data_comm:network_comm.h"
  "//chain/storage/proto:leveldb_config_cc_proto"
  "//chain/storage/proto:leveldb_config_cc_proto" -> "//chain/storage/proto:leveldb_config_proto"
  "//chain/storage/proto:leveldb_config_proto"
  "//chain/storage/proto:leveldb_config_proto" -> "//chain/storage/proto:leveldb_config.proto"
  "//chain/storage/proto:leveldb_config.proto"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:consensus_manager_pbft.cpp\n//platform/consensus/ordering/pbft:consensus_manager_pbft.h"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:checkpoint_manager"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:commitment"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:message_manager"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:performance_manager"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:query"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/ordering/pbft:viewchange_manager"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//common/crypto:signature_verifier"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/consensus/recovery:recovery"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft" -> "//platform/networkstrate:consensus_manager"
  "//platform/networkstrate:consensus_manager"
  "//platform/networkstrate:consensus_manager" -> "//platform/networkstrate:consensus_manager.cpp\n//platform/networkstrate:consensus_manager.h\n//platform/common/queue:blocking_queue"
  "//platform/networkstrate:consensus_manager" -> "//platform/networkstrate:replica_communicator"
  "//platform/networkstrate:consensus_manager" -> "//platform/networkstrate:service_interface"
  "//platform/networkstrate:consensus_manager" -> "//common:comm"
  "//platform/networkstrate:consensus_manager" -> "//platform/config:resdb_config"
  "//platform/networkstrate:consensus_manager" -> "//platform/proto:broadcast_cc_proto"
  "//platform/networkstrate:consensus_manager" -> "//platform/proto:resdb_cc_proto"
  "//platform/networkstrate:consensus_manager" -> "//platform/statistic:stats"
  "//platform/proto:broadcast_cc_proto"
  "//platform/proto:broadcast_cc_proto" -> "//platform/proto:broadcast_proto"
  "//platform/proto:broadcast_proto"
  "//platform/networkstrate:service_interface"
  "//platform/networkstrate:service_interface" -> "//platform/networkstrate:service_interface.h\n//platform/networkstrate:service_interface.cpp"
  "//platform/networkstrate:service_interface" -> "//platform/networkstrate:server_comm"
  "//platform/networkstrate:service_interface" -> "//platform/common/data_comm:data_comm"
  "//platform/networkstrate:service_interface" -> "//platform/config:resdb_config"
  "//platform/common/data_comm:data_comm"
  "//platform/common/data_comm:data_comm" -> "//platform/common/data_comm:data_comm.h"
  "//platform/common/data_comm:data_comm.h"
  "//platform/networkstrate:consensus_manager.cpp\n//platform/networkstrate:consensus_manager.h\n//platform/common/queue:blocking_queue"
  "//platform/consensus/recovery:recovery"
  "//platform/consensus/recovery:recovery" -> "//platform/proto:system_info_data_cc_proto\n//platform/consensus/recovery:recovery.h\n//platform/consensus/recovery:recovery.cpp"
  "//platform/consensus/recovery:recovery" -> "//chain/storage:storage"
  "//platform/consensus/recovery:recovery" -> "//common/utils:utils"
  "//platform/consensus/recovery:recovery" -> "//platform/config:resdb_config"
  "//platform/consensus/recovery:recovery" -> "//platform/consensus/checkpoint:checkpoint"
  "//platform/consensus/recovery:recovery" -> "//platform/consensus/execution:system_info"
  "//platform/consensus/recovery:recovery" -> "//platform/networkstrate:server_comm"
  "//platform/consensus/recovery:recovery" -> "//platform/proto:resdb_cc_proto"
  "//platform/proto:system_info_data_cc_proto\n//platform/consensus/recovery:recovery.h\n//platform/consensus/recovery:recovery.cpp"
  "//platform/consensus/ordering/pbft:viewchange_manager"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/consensus/ordering/pbft:viewchange_manager.cpp\n//platform/proto:viewchange_message_cc_proto\n//platform/consensus/ordering/pbft:viewchange_manager.h"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/consensus/ordering/pbft:checkpoint_manager"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/consensus/ordering/pbft:message_manager"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/consensus/ordering/pbft:transaction_utils"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/config:resdb_config"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/consensus/execution:system_info"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/networkstrate:replica_communicator"
  "//platform/consensus/ordering/pbft:viewchange_manager" -> "//platform/statistic:stats"
  "//platform/consensus/ordering/pbft:viewchange_manager.cpp\n//platform/proto:viewchange_message_cc_proto\n//platform/consensus/ordering/pbft:viewchange_manager.h"
  "//platform/consensus/ordering/pbft:query"
  "//platform/consensus/ordering/pbft:query" -> "//platform/consensus/ordering/pbft:query.cpp\n//platform/consensus/ordering/pbft:query.h\n//executor/common:custom_query"
  "//platform/consensus/ordering/pbft:query" -> "//platform/consensus/ordering/pbft:message_manager"
  "//platform/consensus/ordering/pbft:query" -> "//platform/config:resdb_config"
  "//platform/consensus/ordering/pbft:query" -> "//platform/proto:resdb_cc_proto"
  "//platform/consensus/ordering/pbft:query.cpp\n//platform/consensus/ordering/pbft:query.h\n//executor/common:custom_query"
  "//platform/consensus/ordering/pbft:performance_manager"
  "//platform/consensus/ordering/pbft:performance_manager" -> "//platform/consensus/ordering/pbft:performance_manager.h\n//platform/consensus/ordering/pbft:performance_manager.cpp"
  "//platform/consensus/ordering/pbft:performance_manager" -> "//platform/consensus/ordering/pbft:lock_free_collector_pool"
  "//platform/consensus/ordering/pbft:performance_manager" -> "//platform/consensus/ordering/pbft:transaction_utils"
  "//platform/consensus/ordering/pbft:performance_manager" -> "//platform/networkstrate:replica_communicator"
  "//platform/consensus/ordering/pbft:performance_manager.h\n//platform/consensus/ordering/pbft:performance_manager.cpp"
  "//platform/consensus/ordering/pbft:commitment"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/consensus/ordering/pbft:commitment.cpp\n//platform/common/queue:batch_queue\n//platform/consensus/ordering/pbft:response_manager\n//platform/consensus/execution:duplicate_manager\n//platform/consensus/ordering/pbft:commitment.h"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/consensus/ordering/pbft:message_manager"
  "//platform/consensus/ordering/pbft:commitment" -> "//common/utils:utils"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/config:resdb_config"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/networkstrate:replica_communicator"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/proto:resdb_cc_proto"
  "//platform/consensus/ordering/pbft:commitment" -> "//platform/statistic:stats"
  "//platform/consensus/ordering/pbft:message_manager"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/consensus/ordering/pbft:message_manager.cpp\n//platform/consensus/ordering/pbft:message_manager.h\n//platform/consensus/ordering/pbft:transaction_collector"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/consensus/ordering/pbft:checkpoint_manager"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/consensus/ordering/pbft:lock_free_collector_pool"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/consensus/ordering/pbft:transaction_utils"
  "//platform/consensus/ordering/pbft:message_manager" -> "//chain/state:chain_state"
  "//platform/consensus/ordering/pbft:message_manager" -> "//executor/common:transaction_manager"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/config:resdb_config"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/networkstrate:server_comm"
  "//platform/consensus/ordering/pbft:message_manager" -> "//platform/proto:resdb_cc_proto"
  "//executor/common:transaction_manager"
  "//executor/common:transaction_manager" -> "//executor/common:transaction_manager.cpp\n//executor/common:transaction_manager.h"
  "//executor/common:transaction_manager" -> "//chain/storage:storage"
  "//executor/common:transaction_manager" -> "//common:comm"
  "//executor/common:transaction_manager" -> "//platform/proto:resdb_cc_proto"
  "//executor/common:transaction_manager.cpp\n//executor/common:transaction_manager.h"
  "//platform/consensus/ordering/pbft:lock_free_collector_pool"
  "//platform/consensus/ordering/pbft:message_manager.cpp\n//platform/consensus/ordering/pbft:message_manager.h\n//platform/consensus/ordering/pbft:transaction_collector"
  "//platform/consensus/ordering/pbft:commitment.cpp\n//platform/common/queue:batch_queue\n//platform/consensus/ordering/pbft:response_manager\n//platform/consensus/execution:duplicate_manager\n//platform/consensus/ordering/pbft:commitment.h"
  "//platform/consensus/ordering/pbft:checkpoint_manager"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/consensus/execution:transaction_executor\n//platform/proto:checkpoint_info_cc_proto\n//platform/consensus/ordering/pbft:checkpoint_manager.h\n//interface/common:resdb_txn_accessor\n//platform/consensus/ordering/pbft:checkpoint_manager.cpp"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/consensus/ordering/pbft:transaction_utils"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//chain/state:chain_state"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//common/crypto:signature_verifier"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/config:resdb_config"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/consensus/checkpoint:checkpoint"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/networkstrate:replica_communicator"
  "//platform/consensus/ordering/pbft:checkpoint_manager" -> "//platform/networkstrate:server_comm"
  "//platform/networkstrate:server_comm"
  "//platform/networkstrate:replica_communicator"
  "//platform/consensus/checkpoint:checkpoint"
  "//platform/config:resdb_config"
  "//platform/config:resdb_config" -> "//platform/config:resdb_config.cpp\n//platform/config:resdb_config.h"
  "//platform/config:resdb_config" -> "//common:comm"
  "//platform/config:resdb_config" -> "//platform/proto:replica_info_cc_proto"
  "//platform/config:resdb_config.cpp\n//platform/config:resdb_config.h"
  "//chain/state:chain_state"
  "//platform/consensus/execution:transaction_executor\n//platform/proto:checkpoint_info_cc_proto\n//platform/consensus/ordering/pbft:checkpoint_manager.h\n//interface/common:resdb_txn_accessor\n//platform/consensus/ordering/pbft:checkpoint_manager.cpp"
  "//platform/consensus/ordering/pbft:consensus_manager_pbft.cpp\n//platform/consensus/ordering/pbft:consensus_manager_pbft.h"
  "//chain/storage:memory_db"
  "//chain/storage:memory_db" -> "//chain/storage:memory_db.h\n//chain/storage:memory_db.cpp"
  "//chain/storage:memory_db" -> "//chain/storage:storage"
  "//chain/storage:memory_db" -> "//common:comm"
  "//chain/storage:memory_db.h\n//chain/storage:memory_db.cpp"
  "//common/crypto:signature_verifier"
  "//common/crypto:signature_verifier" -> "//common/proto:signature_info_cc_proto\n//common/crypto:signature_verifier.cpp\n//:cryptopp_lib\n//common/crypto:signature_verifier_interface\n//common/crypto:signature_verifier.h\n//common/crypto:signature_utils"
  "//common/crypto:signature_verifier" -> "//common:comm"
  "//common/proto:signature_info_cc_proto\n//common/crypto:signature_verifier.cpp\n//:cryptopp_lib\n//common/crypto:signature_verifier_interface\n//common/crypto:signature_verifier.h\n//common/crypto:signature_utils"
  "//executor/kv:kv_executor.cpp\n//executor/kv:kv_executor.h"
  "//platform/consensus/execution:system_info"
  "//platform/networkstrate:service_interface.h\n//platform/networkstrate:service_interface.cpp"
  "//platform/consensus/ordering/pbft:transaction_utils"
  "//common:boost_lockfree\n//platform/common/queue:lock_free_queue.h"
  "//chain/storage:lmdb"
  "//chain/storage:lmdb" -> "//chain/storage:lmdb.cpp\n//chain/storage:lmdb.h"
  "//chain/storage:lmdb" -> "//chain/storage:storage"
  "//chain/storage:lmdb" -> "//chain/storage/proto:kv_cc_proto"
  "//chain/storage:lmdb" -> "//common:comm"
  "//chain/storage:storage"
  "//chain/storage:storage" -> "//chain/storage:storage.h"
  "//chain/storage:storage.h"
  "//chain/storage:lmdb.cpp\n//chain/storage:lmdb.h"
  "//chain/storage/proto:kv_cc_proto"
  "//chain/storage/proto:kv_cc_proto" -> "//chain/storage/proto:kv_proto"
  "//chain/storage/proto:kv_proto"
  "//chain/storage/proto:kv_proto" -> "//chain/storage/proto:kv.proto"
  "//chain/storage/proto:kv.proto"
  "//platform/proto:replica_info_cc_proto"
  "//platform/proto:replica_info_cc_proto" -> "//platform/proto:replica_info_proto"
  "//platform/proto:replica_info_proto"
  "//platform/statistic:stats"
  "//platform/statistic:stats" -> "//platform/statistic:stats.cpp\n//third_party:crow\n//common:beast\n//platform/statistic:stats.h\n//common:json\n//platform/statistic:prometheus_handler\n//third_party:prometheus"
  "//platform/statistic:stats" -> "//common:asio"
  "//platform/statistic:stats" -> "//common:comm"
  "//platform/statistic:stats" -> "//common/utils:utils"
  "//platform/statistic:stats" -> "//platform/common/network:tcp_socket"
  "//platform/statistic:stats" -> "//platform/proto:resdb_cc_proto"
  "//platform/statistic:stats" -> "//proto/kv:kv_cc_proto"
  "//proto/kv:kv_cc_proto"
  "//proto/kv:kv_cc_proto" -> "//proto/kv:kv_proto"
  "//proto/kv:kv_proto"
  "//proto/kv:kv_proto" -> "//proto/kv:kv.proto"
  "//proto/kv:kv.proto"
  "//platform/proto:resdb_cc_proto"
  "//platform/proto:resdb_cc_proto" -> "//platform/proto:resdb_proto"
  "//platform/proto:resdb_proto"
  "//platform/common/network:tcp_socket"
  "//platform/common/network:tcp_socket" -> "//platform/common/network:tcp_socket.h\n//platform/common/network:socket\n//platform/common/network:tcp_socket.cpp"
  "//platform/common/network:tcp_socket.h\n//platform/common/network:socket\n//platform/common/network:tcp_socket.cpp"
  "//common/utils:utils"
  "//common:comm"
  "//common:comm" -> "//common:absl"
  "//common:comm" -> "//common:glog"
  "//common:glog"
  "//common:glog" -> "@com_github_google_glog//:glog"
  "@com_github_google_glog//:glog"
  "@com_github_google_glog//:glog" -> "@com_github_google_glog//:src/stacktrace_generic-inl.h\n@com_github_google_glog//:src/signalhandler.cc\n@com_github_google_glog//:src/logging.cc\n@com_github_google_glog//:wasm\n@com_github_google_glog//:src/symbolize.h\n@com_github_google_glog//:src/base/commandlineflags.h\n@com_github_google_glog//:src/symbolize.cc\n@com_github_google_glog//:raw_logging_h\n@com_github_google_glog//:vlog_is_on_h\n@com_github_google_glog//:src/stacktrace_x86-inl.h\n...and 25 more items"
  [label="@bazel_tools//src/conditions:windows"];
  "@com_github_google_glog//:src/stacktrace_generic-inl.h\n@com_github_google_glog//:src/signalhandler.cc\n@com_github_google_glog//:src/logging.cc\n@com_github_google_glog//:wasm\n@com_github_google_glog//:src/symbolize.h\n@com_github_google_glog//:src/base/commandlineflags.h\n@com_github_google_glog//:src/symbolize.cc\n@com_github_google_glog//:raw_logging_h\n@com_github_google_glog//:vlog_is_on_h\n@com_github_google_glog//:src/stacktrace_x86-inl.h\n...and 25 more items"
  "//common:absl"
  "//common:absl" -> "@com_google_absl//absl/status:status"
  "//common:absl" -> "@com_google_absl//absl/status:statusor"
  "@com_google_absl//absl/status:statusor"
  "@com_google_absl//absl/status:statusor" -> "@com_google_absl//absl/utility:utility\n@com_google_absl//absl/status:statusor.cc\n@com_google_absl//absl/status:internal/statusor_internal.h\n@com_google_absl//absl/status:statusor.h\n@com_google_absl//absl/base:base\n@com_google_absl//absl/meta:type_traits\n@com_google_absl//absl/types:variant"
  "@com_google_absl//absl/status:statusor" -> "@com_google_absl//absl/status:status"
  "@com_google_absl//absl/status:statusor" -> "@com_google_absl//absl/strings:strings\n@com_google_absl//absl:msvc_compiler\n@com_google_absl//absl/base:core_headers\n@com_google_absl//absl:clang_compiler\n@com_google_absl//absl/base:raw_logging_internal\n@com_google_absl//absl:clang-cl_compiler"
  "@com_google_absl//absl/utility:utility\n@com_google_absl//absl/status:statusor.cc\n@com_google_absl//absl/status:internal/statusor_internal.h\n@com_google_absl//absl/status:statusor.h\n@com_google_absl//absl/base:base\n@com_google_absl//absl/meta:type_traits\n@com_google_absl//absl/types:variant"
  "@com_google_absl//absl/status:status"
  "@com_google_absl//absl/status:status" -> "@com_google_absl//absl/status:status_payload_printer.h\n@com_google_absl//absl/debugging:symbolize\n@com_google_absl//absl/strings:cord\n@com_google_absl//absl/container:inlined_vector\n@com_google_absl//absl/status:status.h\n@com_google_absl//absl/status:internal/status_internal.h\n@com_google_absl//absl/debugging:stacktrace\n@com_google_absl//absl/status:status_payload_printer.cc\n@com_google_absl//absl/status:status.cc\n@com_google_absl//absl/base:atomic_hook\n...and 4 more items"
  "@com_google_absl//absl/status:status" -> "@com_google_absl//absl/strings:strings\n@com_google_absl//absl:msvc_compiler\n@com_google_absl//absl/base:core_headers\n@com_google_absl//absl:clang_compiler\n@com_google_absl//absl/base:raw_logging_internal\n@com_google_absl//absl:clang-cl_compiler"
  "@com_google_absl//absl/strings:strings\n@com_google_absl//absl:msvc_compiler\n@com_google_absl//absl/base:core_headers\n@com_google_absl//absl:clang_compiler\n@com_google_absl//absl/base:raw_logging_internal\n@com_google_absl//absl:clang-cl_compiler"
  "@com_google_absl//absl/status:status_payload_printer.h\n@com_google_absl//absl/debugging:symbolize\n@com_google_absl//absl/strings:cord\n@com_google_absl//absl/container:inlined_vector\n@com_google_absl//absl/status:status.h\n@com_google_absl//absl/status:internal/status_internal.h\n@com_google_absl//absl/debugging:stacktrace\n@com_google_absl//absl/status:status_payload_printer.cc\n@com_google_absl//absl/status:status.cc\n@com_google_absl//absl/base:atomic_hook\n...and 4 more items"
  "//common:asio"
  "//platform/statistic:stats.cpp\n//third_party:crow\n//common:beast\n//platform/statistic:stats.h\n//common:json\n//platform/statistic:prometheus_handler\n//third_party:prometheus"
}

        `
    ],
];
