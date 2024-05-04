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
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    deps = [
        "//executor/common:transaction_manager",
        "//executor/contract/manager:address_manager",
        "//executor/contract/manager:contract_manager",
        "//platform/config:resdb_config_utils",
        "//proto/contract:rpc_cc_proto",
    ],
)

cc_test(
    name = "contract_executor_test",
    srcs = ["contract_executor_test.cpp"],
    data = [
        "//executor/contract/executor/test_data:contract.json",
    ],
    tags = ["manual"],
    deps = [
        ":contract_executor",
        "//common/test:test_main",
    ],
)
