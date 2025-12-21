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

licenses(["notice"])
exports_files(["LICENSE"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "eEVM",
    srcs = glob(
        ["**/*.cpp"],
        exclude = [
            "3rdparty/intx/test/**",
            "3rdparty/intx/examples/**",
            "samples/**",
            "tests/**",
        ],
    )+ glob(
        ["3rdparty/keccak/*.c"] 
    ),
    hdrs = glob(
        ["**/*.hpp"],
    ) + glob(
        ["**/*.h"],
    ) + glob(
        ["3rdparty/keccak/*.macros"]
    ) + glob(
        ["3rdparty/keccak/*.inc"]
    ),
    includes = [
        ".",
        "3rdparty",
        "include",
        "3rdparty/intx/include",
        "3rdparty/keccak",
    ],
    visibility = ["//visibility:public"],
)
