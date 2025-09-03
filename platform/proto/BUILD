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

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc//python:defs.bzl", "python_proto_library")

proto_library(
    name = "network_type_proto",
    srcs = ["network_type.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "network_type_cc_proto",
    deps = [":network_type_proto"],
)

proto_library(
    name = "replica_info_proto",
    srcs = ["replica_info.proto"],
    deps = [
        "//chain/storage/proto:leveldb_config_proto",
         "//chain/storage/proto:rocksdb_config_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "replica_info_cc_proto",
    deps = [
        ":replica_info_proto",
    ],
)

python_proto_library(
    name = "replica_info_py_proto",
    protos = [
        ":replica_info_proto",
        "//chain/storage/proto:leveldb_config_proto",
        "//chain/storage/proto:rocksdb_config_proto",
    ],
    deps = [
        "//common/proto:signature_info_py_proto",
    ],
)

proto_library(
    name = "resdb_proto",
    srcs = ["resdb.proto"],
    deps = [
        ":replica_info_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "resdb_cc_proto",
    deps = [":resdb_proto"],
)

proto_library(
    name = "client_test_proto",
    srcs = ["client_test.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "client_test_cc_proto",
    deps = [":client_test_proto"],
)

proto_library(
    name = "logging_proto",
    srcs = ["logging.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "logging_cc_proto",
    deps = [":logging_proto"],
)

proto_library(
    name = "checkpoint_info_proto",
    srcs = ["checkpoint_info.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "checkpoint_info_cc_proto",
    deps = [":checkpoint_info_proto"],
)

proto_library(
    name = "viewchange_message_proto",
    srcs = ["viewchange_message.proto"],
    deps = [
        ":checkpoint_info_proto",
        ":resdb_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "viewchange_message_cc_proto",
    deps = [":viewchange_message_proto"],
)

proto_library(
    name = "broadcast_proto",
    srcs = ["broadcast.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "broadcast_cc_proto",
    deps = [":broadcast_proto"],
)

proto_library(
    name = "system_info_data_proto",
    srcs = ["system_info_data.proto"],
)

cc_proto_library(
    name = "system_info_data_cc_proto",
    deps = [
        ":system_info_data_proto",
    ],
)
