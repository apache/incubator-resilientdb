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
    name = "duckdb",
    # The amalgamation zip usually just has these three at the root.
    srcs = ["duckdb.cpp"],
    hdrs = [
        "duckdb.hpp",
        "duckdb.h",
    ],
    includes = ["."],
    # ResilientDB already builds with modern C++; C++17 is safe for DuckDB.
    copts = [
        "-std=c++17",
        # Optional: silence some noisy warnings that big amalgamations usually trigger.
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Wno-sign-compare",
    ],
)




