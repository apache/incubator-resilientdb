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
    name = "absl",
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "asio",
    deps = [
        "@boost//:asio",
    ],
)

cc_library(
    name = "beast",
    deps = [
        "@boost//:beast",
    ],
)

cc_library(
    name = "boost_lockfree",
    deps = [
        "@boost//:lockfree",
    ],
)

cc_library(
    name = "glog",
    deps = [
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "pybind",
    deps = [
        "@pybind11",
    ],
)

cc_library(
    name = "json",
    deps = [
        "@nlohmann_json//:json",
    ],
)

cc_library(
    name = "boost_comm",
    deps = [
        "@boost//:algorithm",
        "@boost//:optional",
    ],
)

cc_library(
    name = "comm",
    deps = [
        ":absl",
        ":glog",
    ],
)
