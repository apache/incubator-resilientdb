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
    name = "transaction_utils",
    srcs = ["transaction_utils.cpp"],
    hdrs = ["transaction_utils.h"],
    deps = [
        "//platform/proto:resdb_cc_proto",
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

cc_library(
    name = "raft_recovery",
    srcs = ["raft_recovery.cpp"],
    hdrs = ["raft_recovery.h"],
    deps = [
        "//chain/storage",
        "//common/utils",
        "//platform/consensus/ordering/raft/proto:proposal_cc_proto",
        "//platform/config:resdb_config",
        "//platform/consensus/checkpoint",
        "//platform/consensus/execution:system_info",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
        "//platform/proto:system_info_data_cc_proto",
        "//platform/consensus/recovery:recovery"
    ],
)

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":checkpoint_manager",
        ":raft_recovery",
        "//common/utils",
        "//platform/consensus/ordering/common/framework:consensus",
        "//platform/consensus/ordering/raft/algorithm:raft",
    ],
)
