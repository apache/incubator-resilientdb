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

package(default_visibility = [
    "//platform:__subpackages__",
    "//service:__subpackages__",
    "//chain/storage:__subpackages__",
])

cc_library(
    name = "stats",
    srcs = ["stats.cpp"],
    hdrs = ["stats.h"],
    deps = [
        ":prometheus_handler",
        "//common:asio",
        "//common:beast",
        "//common:comm",
        "//common:json",
        "//common/utils",
        "//platform/common/network:tcp_socket",
        "//platform/proto:resdb_cc_proto",
        "//proto/kv:kv_cc_proto",
        "//third_party:crow",
        "//third_party:prometheus",
    ],
)

cc_library(
    name = "prometheus_handler",
    srcs = ["prometheus_handler.cpp"],
    hdrs = ["prometheus_handler.h"],
    deps = [
        "//common:comm",
        "//third_party:prometheus",
    ],
)

cc_binary(
    name = "set_random_data",
    srcs = ["set_random_data.cpp"],
)
