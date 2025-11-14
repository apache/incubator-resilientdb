# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "raft_storage",
    srcs = [
        "raft_log.cpp",
        "raft_persistent_state.cpp",
        "raft_snapshot_manager.cpp",
    ],
    hdrs = [
        "raft_log.h",
        "raft_persistent_state.h",
        "raft_snapshot_manager.h",
    ],
    deps = [
        "//chain/storage:prefix_allocator",
        "//chain/storage:storage",
        "//common:comm",
        "//platform/consensus/ordering/raft/proto:raft_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "raft_rpc_lib",
    srcs = ["raft_rpc.cpp"],
    hdrs = [
        "raft_message_type.h",
        "raft_rpc.h",
    ],
    deps = [
        "//common:comm",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/raft/proto:raft_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "consensus_manager_raft",
    srcs = [
        "raft_node.cpp",
        "consensus_manager_raft.cpp",
    ],
    hdrs = [
        "consensus_manager_raft.h",
        "raft_node.h",
        "raft_message_type.h",
    ],
    deps = [
        ":raft_rpc_lib",
        ":raft_storage",
        "//executor/common:custom_query",
        "//executor/common:transaction_manager",
        "//platform/consensus/ordering/raft/proto:raft_cc_proto",
        "//platform/networkstrate:consensus_manager",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "raft_log_test",
    srcs = ["raft_log_test.cpp"],
    deps = [
        ":raft_storage",
        "//chain/storage:memory_db",
        "//chain/storage:prefix_allocator",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "raft_persistent_state_test",
    srcs = ["raft_persistent_state_test.cpp"],
    deps = [
        ":raft_storage",
        "//chain/storage:memory_db",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "raft_snapshot_manager_test",
    srcs = ["raft_snapshot_manager_test.cpp"],
    deps = [
        ":raft_storage",
        "//chain/storage:memory_db",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "raft_rpc_test",
    srcs = ["raft_rpc_test.cpp"],
    deps = [
        ":raft_rpc_lib",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)
