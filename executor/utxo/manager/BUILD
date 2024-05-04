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

package(default_visibility = ["//executor/utxo:__subpackages__"])

cc_library(
    name = "tx_mempool",
    srcs = ["tx_mempool.cpp"],
    hdrs = ["tx_mempool.h"],
    deps = [
        "//common:comm",
        "//proto/utxo:utxo_cc_proto",
    ],
)

cc_test(
    name = "tx_mempool_test",
    srcs = ["tx_mempool_test.cpp"],
    deps = [
        ":tx_mempool",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "wallet",
    srcs = ["wallet.cpp"],
    hdrs = ["wallet.h"],
    deps = [
        "//common:comm",
        "//proto/utxo:utxo_cc_proto",
    ],
)

cc_test(
    name = "wallet_test",
    srcs = ["wallet_test.cpp"],
    deps = [
        ":wallet",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "transaction",
    srcs = ["transaction.cpp"],
    hdrs = ["transaction.h"],
    deps = [
        ":tx_mempool",
        ":wallet",
        "//common:comm",
        "//common/crypto:hash",
        "//common/crypto:signature_utils",
        "//proto/utxo:config_cc_proto",
        "//proto/utxo:utxo_cc_proto",
    ],
)

cc_test(
    name = "transaction_test",
    srcs = ["transaction_test.cpp"],
    deps = [
        ":transaction",
        "//common/crypto:key_generator",
        "//common/test:test_main",
    ],
)
