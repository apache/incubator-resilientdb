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

load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make", "make")

cc_library(
    name = "prometheus",
    deps = [
        "@com_github_jupp0r_prometheus_cpp//:prometheus",
    ],
)

cc_library(
    name = "snappy",
    deps = [
        "@com_google_snappy//:snappy",
    ],
)

cc_library(
    name = "leveldb",
    deps = [
        "@com_google_leveldb//:leveldb",
    ],
)

cc_library(
    name = "rocksdb",
    # tags = ["manual"],
    deps = [
        "@com_github_facebook_rocksdb//:rocksdb"
    ],
)

make(
    name = "zstd",
    lib_source = "@com_facebook_zstd//:all_srcs",
    out_static_libs = ["libzstd.a"],
)

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

cc_library(
    name = "evm_lib",
    deps = [
        "@eEVM",
    ],
)

cc_library(
    name = "crow",
    deps = [
        "@com_crowcpp_crow//:crow",
    ],
)


