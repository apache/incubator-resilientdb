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
    name = "resdb_state_accessor",
    srcs = ["resdb_state_accessor.cpp"],
    hdrs = ["resdb_state_accessor.h"],
    deps = [
        "//common:comm",
        "//interface/rdbc:net_channel",
        "//platform/proto:replica_info_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "resdb_state_accessor_test",
    srcs = ["resdb_state_accessor_test.cpp"],
    deps = [
        ":resdb_state_accessor",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
    ],
)

cc_library(
    name = "resdb_txn_accessor",
    srcs = ["resdb_txn_accessor.cpp"],
    hdrs = ["resdb_txn_accessor.h"],
    deps = [
        "//common:comm",
        "//interface/rdbc:net_channel",
        "//platform/proto:replica_info_cc_proto",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_resdb_txn_accessor",
    testonly = True,
    hdrs = ["mock_resdb_txn_accessor.h"],
    deps = [
        ":resdb_txn_accessor",
    ],
)

cc_test(
    name = "resdb_txn_accessor_test",
    srcs = ["resdb_txn_accessor_test.cpp"],
    deps = [
        ":resdb_txn_accessor",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/config:resdb_config_utils",
    ],
)
