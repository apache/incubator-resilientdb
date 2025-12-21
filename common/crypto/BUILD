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
    name = "key_generator",
    srcs = ["key_generator.cpp"],
    hdrs = ["key_generator.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
        "//common/proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "hash",
    srcs = ["hash.cpp"],
    hdrs = ["hash.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
    ],
)

cc_test(
    name = "hash_test",
    srcs = ["hash_test.cpp"],
    deps = [
        ":hash",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "signature_utils",
    srcs = ["signature_utils.cpp"],
    hdrs = ["signature_utils.h"],
    deps = [
        "//:cryptopp_lib",
        "//common:comm",
        "//common/proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "signature_verifier_interface",
    srcs = ["signature_verifier_interface.cpp"],
    hdrs = ["signature_verifier_interface.h"],
    deps = [
        "//common:comm",
        "//common/proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "signature_verifier",
    srcs = ["signature_verifier.cpp"],
    hdrs = ["signature_verifier.h"],
    deps = [
        ":signature_utils",
        ":signature_verifier_interface",
        "//:cryptopp_lib",
        "//common:comm",
        "//common/proto:signature_info_cc_proto",
    ],
)

cc_library(
    name = "mock_signature_verifier",
    testonly = True,
    hdrs = ["mock_signature_verifier.h"],
    deps = [
        ":signature_verifier",
    ],
)

cc_test(
    name = "signature_verifier_test",
    srcs = ["signature_verifier_test.cpp"],
    deps = [
        ":key_generator",
        ":signature_verifier",
        "//:cryptopp_lib",
        "//common/test:test_main",
    ],
)
