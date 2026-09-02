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
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "thunderbolt_service",
    srcs = ["thunderbolt_service.cpp"],
    hdrs = ["thunderbolt_service.h"],
    deps = [
        ":contract_transaction_manager",
        ":execution_helper",
        "//platform/consensus/ordering/thunderbolt/framework:consensus",
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:contract_manager",
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:leveldb_storage",
        "//proto/contract:rpc_cc_proto",
    ],
)

cc_library(
    name = "execution_helper",
    srcs = ["execution_helper.cpp"],
    hdrs = ["execution_helper.h"],
    deps = [
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:contract_manager",
        "//proto/contract:rpc_cc_proto",
    ],
)

cc_library(
    name = "contract_transaction_manager",
    srcs = ["contract_transaction_manager.cpp"],
    hdrs = ["contract_transaction_manager.h"],
    deps = [
        ":execution_helper",
        "//executor/common:transaction_manager",
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:contract_manager",
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:leveldb_storage",
        "//platform/consensus/ordering/thunderbolt/executor/manager:streaming_committer",
        "//proto/contract:rpc_cc_proto",
    ],
)

cc_binary(
    name = "contract_server",
    srcs = ["contract_server.cpp"],
    deps = [
        ":thunderbolt_service",
        "//platform/config:resdb_config_utils",
        "//service/contract/benchmark/generator:string_generator",
        "//service/contract/benchmark/generator:zipfian_generator",
        "//platform/consensus/ordering/thunderbolt/executor/x_manager:leveldb_storage",
        "//service/contract/thunderbolt_service:contract_transaction_manager",
        "//service/utils:server_factory",
    ],
)
