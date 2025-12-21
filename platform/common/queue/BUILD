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
    name = "blocking_queue",
    hdrs = [
        "blocking_queue.h",
    ],
)

cc_library(
    name = "lock_free_queue",
    hdrs = [
        "lock_free_queue.h",
    ],
    deps = [
        "//common:boost_lockfree",
        "//common:comm",
    ],
)

cc_test(
    name = "lock_free_queue_test",
    srcs = ["lock_free_queue_test.cpp"],
    deps = [
        ":lock_free_queue",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "batch_queue",
    hdrs = [
        "batch_queue.h",
    ],
    deps = [
        "//common:comm",
    ],
)

cc_test(
    name = "batch_queue_test",
    srcs = ["batch_queue_test.cpp"],
    deps = [
        ":batch_queue",
        "//common/test:test_main",
    ],
)
