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
    name = "net_channel",
    srcs = ["net_channel.cpp"],
    hdrs = ["net_channel.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/data_comm",
        "//platform/common/network:tcp_socket",
        "//platform/config:resdb_config",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_net_channel",
    testonly = True,
    hdrs = ["mock_net_channel.h"],
    deps = [
        ":net_channel",
    ],
)

cc_test(
    name = "net_channel_test",
    srcs = ["net_channel_test.cpp"],
    deps = [
        ":net_channel",
        "//:cryptopp_lib",
        "//common/crypto:key_generator",
        "//common/test:test_main",
        "//platform/common/network:mock_socket",
        "//platform/proto:client_test_cc_proto",
    ],
)

cc_library(
    name = "transaction_constructor",
    srcs = ["transaction_constructor.cpp"],
    hdrs = ["transaction_constructor.h"],
    deps = [
        ":net_channel",
        "//platform/common/data_comm",
    ],
)

cc_test(
    name = "transaction_constructor_test",
    srcs = ["transaction_constructor_test.cpp"],
    deps = [
        ":transaction_constructor",
        "//common/crypto:signature_verifier",
        "//common/test:test_main",
        "//platform/common/network:mock_socket",
        "//platform/proto:client_test_cc_proto",
    ],
)
