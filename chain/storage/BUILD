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
    name = "storage",
    hdrs = ["storage.h"],
    deps = [
    ],
)

cc_library(
    name = "mock_storage",
    hdrs = ["mock_storage.h"],
    deps = [
        ":storage",
    ],
)

cc_library(
    name = "memory_db",
    srcs = ["memory_db.cpp"],
    hdrs = ["memory_db.h"],
    deps = [
        ":storage",
        "//common:comm",
    ],
)

cc_library(
    name = "leveldb",
    srcs = ["leveldb.cpp"],
    hdrs = ["leveldb.h"],
    deps = [
        ":storage",
        "//chain/storage/proto:kv_cc_proto",
        "//chain/storage/proto:leveldb_config_cc_proto",
        "//common:comm",
        "//common/lru:lru_cache",
        "//platform/statistic:stats",
        "//third_party:leveldb",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_library(
    name = "ipfs_client",
    srcs = ["ipfs_client.cpp"],
    hdrs = ["ipfs_client.h"],
    deps = [
        ":storage",
        "//chain/storage/proto:ipfs_config_cc_proto",
        "//common:comm",
        "@boost//:asio",
        "@boost//:beast",
    ],
)

cc_library(
    name = "tiered_storage",
    srcs = ["tiered_storage.cpp"],
    hdrs = ["tiered_storage.h"],
    deps = [
        ":leveldb",
        ":storage",
        ":ipfs_client",
        "//chain/storage/proto:ipfs_config_cc_proto",
        "//chain/storage/proto:tiered_index_cc_proto",
        "//common:comm",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "kv_storage_test",
    size = "small",  # Set the size to "small"
    timeout = "short",  # Set the timeout to "short"
    srcs = ["kv_storage_test.cpp"],
    deps = [
        ":leveldb",
        ":memory_db",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "leveldb_test",
    size = "small",  # Set the size to "small"
    timeout = "short",  # Set the timeout to "short"
    srcs = ["leveldb_test.cpp"],
    deps = [
        ":leveldb",
        "//common/test:test_main",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "tiered_storage_test",
    size = "small",
    timeout = "short",
    srcs = ["tiered_storage_test.cpp"],
    deps = [
        ":leveldb",
        ":memory_db",
        ":tiered_storage",
        ":ipfs_client",
        "//common/test:test_main",
    ],
)

cc_binary(
    name = "ipfs_integration_test",
    srcs = ["ipfs_integration_test.cpp"],
    deps = [
        ":ipfs_client",
        "//chain/storage/proto:ipfs_config_cc_proto",
    ],
)
