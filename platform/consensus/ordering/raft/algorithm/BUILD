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

package(default_visibility = ["//platform/consensus/ordering/raft:__subpackages__"])

cc_library(
    name = "raft",
    srcs = [
        "raft.cpp",
        "leaderelection_manager.cpp",
    ],
    hdrs = [
        "raft.h",
        "leaderelection_manager.h",
    ],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/consensus/ordering/raft/proto:proposal_cc_proto",
        "//platform/statistic:stats",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:replica_communicator",
        "//platform/proto:viewchange_message_cc_proto",
    ],
)

cc_library(
    name = "mock_raft",
    hdrs = ["mock_raft.h"],
    deps = [
        ":raft",
    ],
)

cc_library(
    name = "mock_leader_election_manager",
    hdrs = ["mock_leader_election_manager.h"],
    deps = [
        ":raft",
    ],
)

cc_test(
    name = "leader_election_test",
    srcs = ["leader_election_manager_test.cpp"],
    deps = [
        ":raft",
        ":mock_raft",
        "//platform/config:resdb_config_utils",
        "//common/test:test_main"
    ],
    size="small"
)

cc_test(
    name = "raft_test_append_entries",
    srcs = [
        "raft_test_append_entries.cpp",
        "raft_tests.h",
    ],
    copts = ["-DRAFT_TEST_MODE"],
    deps = [
        ":raft",
        ":mock_leader_election_manager",
        "//platform/networkstrate:mock_replica_communicator",
        "//common/crypto:mock_signature_verifier",
        "//platform/config:resdb_config_utils",
        "//common/test:test_main",
        "//platform/proto:client_test_cc_proto",
    ],
    size="small"
)
