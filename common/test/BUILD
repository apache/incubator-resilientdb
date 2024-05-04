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
    name = "test",
    hdrs = [
        "test_macros.h",
    ],
    deps = [
        "//common:comm",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "test_main",
    defines = ["TEST_MODE"],
    deps = [
        ":test",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
    visibility = ["//visibility:private"],
)

cc_proto_library(
    name = "test_cc_proto",
    visibility = ["//visibility:private"],
    deps = [":test_proto"],
)

cc_test(
    name = "json_test",
    srcs = ["json_test.cpp"],
    deps = [
        ":test_cc_proto",
        ":test_main",
    ],
)
