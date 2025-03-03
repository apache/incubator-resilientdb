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
    name = "crow_service",
    srcs = ["crow_service.cpp"],
    hdrs = ["crow_service.h"],
    deps = [
        "//service/kv_service:resdb_kv_client",
        "//third_party:crow",
        "//third_party:rapidjson",
        "@com_resdb_nexres//common:asio",
        "@com_resdb_nexres//common:boost_comm",
        "@com_resdb_nexres//common/proto:signature_info_cc_proto",
        "@com_resdb_nexres//interface/common:resdb_state_accessor",
        "@com_resdb_nexres//interface/common:resdb_txn_accessor",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
    ],
)

cc_binary(
    name = "crow_service_main",
    srcs = ["crow_service_main.cpp"],
    deps = [
        ":crow_service",
    ],
)

cc_test(
    name = "crow_service_test",
    srcs = ["crow_service_test.cpp"],
    deps = [
        ":crow_service",
        "@com_resdb_nexres//common/test:test_main",
    ],
)
