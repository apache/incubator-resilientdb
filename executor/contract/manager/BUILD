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
    name = "utils",
    hdrs = ["utils.h"],
    deps = [
        "//third_party:evm_lib",
    ],
)

cc_library(
    name = "contract_manager",
    srcs = ["contract_manager.cpp"],
    hdrs = ["contract_manager.h"],
    deps = [
        ":address_manager",
        ":utils",
        "//common:comm",
        "//proto/contract:func_params_cc_proto",
    ],
)

cc_test(
    name = "contract_manager_test",
    srcs = ["contract_manager_test.cpp"],
    data = [
        "//executor/contract/manager/test_data:contract.json",
    ],
    tags = ["manual"],
    deps = [
        ":contract_manager",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "address_manager",
    srcs = ["address_manager.cpp"],
    hdrs = ["address_manager.h"],
    deps = [
        ":utils",
        "//common:comm",
    ],
)

cc_test(
    name = "address_manager_test",
    srcs = ["address_manager_test.cpp"],
    deps = [
        ":address_manager",
        "//common/test:test_main",
    ],
)
