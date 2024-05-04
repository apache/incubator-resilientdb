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
    name = "consensus_manager_geo_pbft",
    srcs = ["consensus_manager_geo_pbft.cpp"],
    hdrs = ["consensus_manager_geo_pbft.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":geo_pbft_commitment",
        "//platform/consensus/execution:geo_transaction_executor",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
    ],
)

cc_library(
    name = "geo_pbft_commitment",
    srcs = ["geo_pbft_commitment.cpp"],
    hdrs = ["geo_pbft_commitment.h"],
    deps = [
        ":hash_set",
        "//common:comm",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:geo_global_executor",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/ordering/common:transaction_utils",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "hash_set",
    hdrs = ["hash_set.h"],
)

cc_test(
    name = "geo_pbft_commitment_test",
    srcs = ["geo_pbft_commitment_test.cpp"],
    tags = ["manual"],
    deps = [
        ":geo_pbft_commitment",
        "//common/crypto:mock_signature_verifier",
        "//common/test:test_main",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:mock_geo_global_executor",
        "//platform/consensus/execution:mock_transaction_executor_impl",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)
