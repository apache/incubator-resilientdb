#
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
#

package(default_visibility = ["//visibility:private"])

cc_library(
    name = "lock_free_collector_pool",
    srcs = ["lock_free_collector_pool.cpp"],
    hdrs = ["lock_free_collector_pool.h"],
    deps = [
        ":transaction_collector",
    ],
)

cc_test(
    name = "lock_free_collector_pool_test",
    srcs = ["lock_free_collector_pool_test.cpp"],
    deps = [
        ":lock_free_collector_pool",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_test(
    name = "response_manager_test",
    srcs = ["response_manager_test.cpp"],
    deps = [
        ":response_manager",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":lock_free_collector_pool",
        ":transaction_collector",
        ":transaction_utils",
        "//chain/state:chain_state",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "transaction_utils",
    srcs = ["transaction_utils.cpp"],
    hdrs = ["transaction_utils.h"],
    deps = [
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":message_manager",
        ":response_manager",
        "//common/utils",
        "//platform/common/queue:batch_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:duplicate_manager",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "commitment_test",
    srcs = ["commitment_test.cpp"],
    deps = [
        ":commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)

cc_library(
    name = "checkpoint_manager",
    srcs = ["checkpoint_manager.cpp"],
    hdrs = ["checkpoint_manager.h"],
    deps = [
        ":transaction_utils",
        "//chain/state:chain_state",
        "//common/crypto:signature_verifier",
        "//interface/common:resdb_txn_accessor",
        "//platform/config:resdb_config",
        "//platform/consensus/checkpoint",
        "//platform/consensus/execution:transaction_executor",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
        "//platform/proto:checkpoint_info_cc_proto",
    ],
)

cc_test(
    name = "checkpoint_manager_test",
    srcs = ["checkpoint_manager_test.cpp"],
    deps = [
        ":checkpoint_manager",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "viewchange_manager",
    srcs = ["viewchange_manager.cpp"],
    hdrs = ["viewchange_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":message_manager",
        ":transaction_utils",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:viewchange_message_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "viewchange_manager_test",
    srcs = ["viewchange_manager_test.cpp"],
    tags = ["manual"],
    deps = [
        ":viewchange_manager",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)

cc_library(
    name = "query",
    srcs = ["query.cpp"],
    hdrs = ["query.h"],
    deps = [
        ":message_manager",
        "//executor/common:custom_query",
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "query_test",
    srcs = ["query_test.cpp"],
    deps = [
        ":commitment",
        ":query",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/consensus/execution:transaction_executor",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "transaction_collector_test",
    srcs = ["transaction_collector_test.cpp"],
    deps = [
        ":transaction_collector",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "consensus_manager_pbft",
    srcs = ["consensus_manager_pbft.cpp"],
    hdrs = ["consensus_manager_pbft.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":checkpoint_manager",
        ":commitment",
        ":message_manager",
        ":performance_manager",
        ":query",
        ":viewchange_manager",
        "//common/crypto:signature_verifier",
        "//platform/consensus/recovery",
        "//platform/networkstrate:consensus_manager",
    ],
)

cc_library(
    name = "pre_very_consensus_manager_pbft",
    hdrs = ["pre_very_consensus_manager_pbft.h"],
    visibility = [
        "//platform:__subpackages__",
        "//service:__subpackages__",
    ],
    deps = [
        ":consensus_manager_pbft",
    ],
)
