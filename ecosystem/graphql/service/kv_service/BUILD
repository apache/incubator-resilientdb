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
#

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_kv_client",
    srcs = ["resdb_kv_client.cpp"],
    hdrs = ["resdb_kv_client.h"],
    deps = [
        "//service/kv_service/proto:kv_server_cc_proto",
        "@com_resdb_nexres//interface/rdbc:transaction_constructor",
    ],
)

cc_library(
    name = "py_verificator",
    srcs = ["py_verificator.cpp"],
    hdrs = ["py_verificator.h"],
    deps = [
        "@com_resdb_nexres//common:comm",
        "@pybind11//:pybind11_embed",
    ],
)

cc_library(
    name = "kv_service_transaction_manager",
    srcs = ["kv_service_transaction_manager.cpp"],
    hdrs = ["kv_service_transaction_manager.h"],
    deps = [
        ":py_verificator",
        "//service/kv_service/proto:kv_server_cc_proto",
        "@com_resdb_nexres//chain/state:chain_state",
        "@com_resdb_nexres//common:comm",
        "@com_resdb_nexres//executor/common:transaction_manager",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
    ],
)

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        ":kv_service_transaction_manager",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "@com_resdb_nexres//service/utils:server_factory",
    ],
)

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//service/kv_service:resdb_kv_client",
        "@com_resdb_nexres//common/proto:signature_info_cc_proto",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
    ],
)
