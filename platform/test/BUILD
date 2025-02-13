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

package(default_visibility = ["//visibility:private"])

cc_test(
    name = "resdb_test",
    srcs = ["resdb_test.cpp"],
    data = [
        "//platform/test/test_data",
    ],
    deps = [
        "//common:comm",
        "//common/test:test_main",
        "//executor/common:transaction_manager",
        "//interface/rdbc:transaction_constructor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//platform/networkstrate:service_network",
        "//platform/test/proto:resdb_test_cc_proto",
    ],
)
