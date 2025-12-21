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
    name = "leveldb",
    srcs = glob(
        ["**/*.cc"],
        exclude = [
            "doc/**",
            "**/*_test.cc",
            "db/leveldbutil.cc",
            "db/db_bench.cc",
            "benchmarks/*.cc",
            "util/env_windows.cc",
            "util/testutil.cc",
        ],
    ),
    hdrs = glob(
        ["**/*.h"],
        exclude = [
          "doc/**",
          "util/testutil.h",
          ],
    ),
    defines = [
        "LEVELDB_PLATFORM_POSIX=1",
        "LEVELDB_IS_BIG_ENDIAN=0",
    ],
    includes = [
        ".",
        "include",
    ],
)
