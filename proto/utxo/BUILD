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

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "utxo_proto",
    srcs = ["utxo.proto"],
)

cc_proto_library(
    name = "utxo_cc_proto",
    deps = [":utxo_proto"],
)

proto_library(
    name = "config_proto",
    srcs = ["config.proto"],
    deps = ["utxo_proto"],
)

cc_proto_library(
    name = "config_cc_proto",
    deps = [":config_proto"],
)

proto_library(
    name = "rpc_proto",
    srcs = ["rpc.proto"],
    deps = ["utxo_proto"],
)

cc_proto_library(
    name = "rpc_cc_proto",
    deps = [":rpc_proto"],
)
