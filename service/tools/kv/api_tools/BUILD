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

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//common/proto:signature_info_cc_proto",
        "//interface/kv:kv_client",
        "//platform/config:resdb_config_utils",
    ],
)

cc_binary(
    name = "kv_client_txn_tools",
    srcs = ["kv_client_txn_tools.cpp"],
    deps = [
        "//interface/common:resdb_txn_accessor",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
    ],
)
