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

cc_binary(
    name = "key_tester_utils.so",
    srcs = ["key_tester_utils.cpp"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//common:pybind",
        "//common/crypto:signature_utils",
    ],
)

py_library(
    name = "key_tester_utils_so",
    data = [":key_tester_utils.so"],
    imports = ["."],
)

py_binary(
    name = "key_tester",
    srcs = ["key_tester.py"],
    legacy_create_init = False,
    python_version = "PY3",
    deps = [
        ":key_tester_utils_so",
        "//service/tools/utxo/wallet_tool/pybind:wallet_tools_py_so",
    ],
)
