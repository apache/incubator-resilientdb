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

cc_library(
    name = "commitment_3pc",
    srcs = ["commitment_3pc.cpp"],
    hdrs = ["commitment_3pc.h"],
    visibility = [
        "//platform/consensus/ordering/sharded_3pc:__pkg__",
    ],
    deps = [
        "//platform/consensus/ordering/pbft:commitment",
    ],
)

cc_library(
    name = "consensus_manager_3pc",
    srcs = ["consensus_manager_3pc.cpp"],
    hdrs = ["consensus_manager_3pc.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":commitment_3pc",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//platform/consensus/recovery",
        "//platform/networkstrate:consensus_manager",
    ],
)
