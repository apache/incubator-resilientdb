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
    name = "resdb_config",
    srcs = ["resdb_config.cpp"],
    hdrs = ["resdb_config.h"],
    deps = [
        "//common:comm",
        "//platform/proto:replica_info_cc_proto",
    ],
)

cc_test(
    name = "resdb_config_test",
    srcs = ["resdb_config_test.cpp"],
    deps = [
        ":resdb_config",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "resdb_config_utils",
    srcs = ["resdb_config_utils.cpp"],
    hdrs = ["resdb_config_utils.h"],
    deps = [
        ":resdb_config",
    ],
)

cc_library(
    name = "resdb_poc_config",
    srcs = ["resdb_poc_config.cpp"],
    hdrs = ["resdb_poc_config.h"],
    deps = [
        ":resdb_config",
    ],
)
