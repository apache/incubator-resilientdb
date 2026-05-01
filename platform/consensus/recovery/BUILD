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

package(default_visibility = ["//platform/consensus:__subpackages__"])

cc_library(
    name = "recovery_base",
    hdrs = ["recovery.h", "recovery_impl.h"],
    deps = [
        "//chain/storage",
        "//common/utils",
        "//platform/config:resdb_config",
        "//platform/consensus/checkpoint",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "recovery",
    srcs = ["pbft_recovery.cpp"],
    hdrs = ["pbft_recovery.h"],
    deps = [
        ":recovery_base",
        "//platform/consensus/execution:system_info",
        "//platform/proto:system_info_data_cc_proto",
    ],
)

cc_test(
    name = "recovery_test",
    srcs = ["recovery_test.cpp"],
    deps = [
        ":recovery",
        "//chain/storage:mock_storage",
        "//common/test:test_main",
        "//platform/consensus/checkpoint:mock_checkpoint",
        "//platform/consensus/ordering/common:transaction_utils",
    ],
)

cc_library(
    name = "mock_raft_recovery",
    hdrs = ["mock_raft_recovery.h"],
    testonly = True,
    deps = [
        ":raft_recovery",
        "//chain/storage:mock_storage",
        "//platform/consensus/checkpoint:mock_checkpoint"
    ],
)

cc_library(
    name = "raft_recovery",
    srcs = ["raft_recovery.cpp"],
    hdrs = ["raft_recovery.h"],
    copts = ["-DRAFT_TEST_MODE"],
    deps = [
        "//chain/storage",
        "//common/utils",
        "//platform/consensus/ordering/raft/proto:proposal_cc_proto",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/raft/framework:raft_checkpoint_manager",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
        "//platform/consensus/recovery:recovery_base"
    ],
)

cc_test(
    name = "raft_recovery_test",
    srcs = [
        "raft_recovery_test.cpp",
    ],
    copts = ["-DRAFT_TEST_MODE"],
    deps = [
        ":raft_recovery",
        "//chain/storage:mock_storage",
        "//platform/consensus/ordering/raft/proto:proposal_cc_proto",
        "//platform/consensus/checkpoint:mock_checkpoint",
        "//platform/consensus/ordering/common:transaction_utils",
        "//common/test:test_main",
        "//platform/proto:client_test_cc_proto",
        "//platform/consensus/ordering/raft/algorithm:raft"
    ],
    size="small"
)
