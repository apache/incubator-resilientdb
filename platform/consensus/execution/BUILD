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
    name = "system_info",
    srcs = ["system_info.cpp"],
    hdrs = ["system_info.h"],
    deps = [
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "system_info_test",
    srcs = ["system_info_test.cpp"],
    deps = [
        ":system_info",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "transaction_executor",
    srcs = ["transaction_executor.cpp"],
    hdrs = ["transaction_executor.h"],
    deps = [
        ":duplicate_manager",
        ":system_info",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "transaction_executor_test",
    srcs = ["transaction_executor_test.cpp"],
    deps = [
        ":system_info",
        ":transaction_executor",
        "//common/test:test_main",
        "//executor/common:mock_transaction_manager",
    ],
)

cc_library(
    name = "geo_transaction_executor",
    srcs = ["geo_transaction_executor.cpp"],
    hdrs = ["geo_transaction_executor.h"],
    deps = [
        ":system_info",
        ":transaction_executor",
        "//platform/consensus/ordering/common:transaction_utils",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "geo_global_executor",
    srcs = ["geo_global_executor.cpp"],
    hdrs = ["geo_global_executor.h"],
    deps = [
        "//executor/common:transaction_manager",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "geo_global_executor_test",
    srcs = ["geo_global_executor_test.cpp"],
    deps = [
        ":geo_global_executor",
        "//common/test:test_main",
        "//executor/common:mock_transaction_manager",
        "//platform/config:resdb_config_utils",
    ],
)

cc_library(
    name = "mock_geo_global_executor",
    hdrs = ["mock_geo_global_executor.h"],
    deps = [
        ":geo_global_executor",
    ],
)

cc_test(
    name = "geo_transaction_executor_test",
    srcs = ["geo_transaction_executor_test.cpp"],
    deps = [
        ":geo_transaction_executor",
        "//common/test",
        "//common/test:test_main",
        "//executor/common:mock_transaction_manager",
        "//platform/config:resdb_config_utils",
        "//platform/networkstrate:mock_replica_communicator",
    ],
)

cc_library(
    name = "duplicate_manager",
    srcs = ["duplicate_manager.cpp"],
    hdrs = ["duplicate_manager.h"],
    deps = [
        "//common:comm",
        "//common/utils",
        "//platform/config:resdb_config",
    ],
)
