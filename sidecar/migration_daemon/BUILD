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
    name = "migration_daemon",
    srcs = [
        "main.cpp",
        "migration_daemon.cpp",
        "migration_daemon.h",
    ],
    deps = [
        "//chain/storage:leveldb",
        "//chain/storage:ipfs_client",
        "//chain/storage:storage",
        "//chain/storage/proto:ipfs_config_cc_proto",
        "//chain/storage/proto:tiered_index_cc_proto",
        "//common:comm",
    ],
)