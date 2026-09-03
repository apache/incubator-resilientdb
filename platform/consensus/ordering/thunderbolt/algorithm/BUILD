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
package(default_visibility = ["//platform/consensus/ordering/thunderbolt:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/thunderbolt/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "thunderbolt",
    srcs = ["thunderbolt.cpp"],
    hdrs = ["thunderbolt.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/statistic:stats",
    ],
)
