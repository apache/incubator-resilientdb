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
    name = "contract_transaction_manager",
    srcs = ["contract_transaction_manager.cpp"],
    hdrs = ["contract_transaction_manager.h"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/thunderbolt/executor/manager:address_manager",
        "//platform/consensus/ordering/thunderbolt/executor/manager:contract_manager",
        "//proto/contract:rpc_cc_proto",
        "//service/utils:server_factory",
    ],
)

cc_test(
    name = "contract_transaction_manager_test",
    srcs = ["contract_transaction_manager_test.cpp"],
    data = [
        "//platform/consensus/ordering/thunderbolt/executor/eo_service/test_data:contract.json",
    ],
    deps = [
        ":contract_transaction_manager",
        "//common/test:test_main",
    ],
)
