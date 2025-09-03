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
    name = "composite_key_benchmark",
    srcs = ["composite_key_benchmark.cpp"],
    deps = [
        "//executor/kv:kv_executor",
        "//chain/storage:leveldb",
        "//chain/storage:rocksdb",
        "//proto/kv:kv_cc_proto",
        "@com_google_benchmark//:benchmark",
        "@nlohmann_json//:json",
        "@com_github_google_glog//:glog",
    ],
    copts = [
        "-std=c++17",
        "-O2",
    ],
)
