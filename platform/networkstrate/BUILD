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

package(default_visibility = ["//platform:__subpackages__"])

cc_library(
    name = "server_comm",
    hdrs = ["server_comm.h"],
    deps = [
        "//interface/rdbc:net_channel",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "service_interface",
    srcs = ["service_interface.cpp"],
    hdrs = ["service_interface.h"],
    deps = [
        ":server_comm",
        "//platform/common/data_comm",
        "//platform/config:resdb_config",
    ],
)

cc_library(
    name = "mock_service_interface",
    hdrs = ["mock_service_interface.h"],
    deps = [
        ":service_interface",
        "//common/test",
    ],
)

cc_library(
    name = "consensus_manager",
    srcs = ["consensus_manager.cpp"],
    hdrs = ["consensus_manager.h"],
    deps = [
        ":replica_communicator",
        ":service_interface",
        "//common:comm",
        "//platform/common/queue:blocking_queue",
        "//platform/config:resdb_config",
        "//platform/proto:broadcast_cc_proto",
        "//platform/proto:resdb_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "consensus_manager_test",
    srcs = ["consensus_manager_test.cpp"],
    deps = [
        ":consensus_manager",
        ":mock_replica_communicator",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "async_acceptor",
    srcs = ["async_acceptor.cpp"],
    hdrs = ["async_acceptor.h"],
    deps = [
        "//common:asio",
        "//common:comm",
        "//platform/config:resdb_config",
    ],
)

cc_test(
    name = "async_acceptor_test",
    srcs = ["async_acceptor_test.cpp"],
    deps = [
        ":async_acceptor",
        "//common/test:test_main",
        "//platform/common/network:tcp_socket",
    ],
)

cc_library(
    name = "service_network",
    srcs = ["service_network.cpp"],
    hdrs = ["service_network.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":async_acceptor",
        ":service_interface",
        "//platform/common/data_comm",
        "//platform/common/data_comm:network_comm",
        "//platform/common/network:tcp_socket",
        "//platform/common/queue:lock_free_queue",
        "//platform/proto:broadcast_cc_proto",
        "//platform/rdbc:acceptor",
        "//platform/statistic:stats",
    ],
)

cc_test(
    name = "service_network_test",
    srcs = ["service_network_test.cpp"],
    tags = ["manual"],
    deps = [
        ":mock_service_interface",
        ":service_network",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "async_replica_client",
    srcs = ["async_replica_client.cpp"],
    hdrs = ["async_replica_client.h"],
    deps = [
        "//common:asio",
        "//interface/rdbc:net_channel",
        "//platform/common/queue:blocking_queue",
        "//platform/common/queue:lock_free_queue",
        "//platform/proto:broadcast_cc_proto",
    ],
)

cc_test(
    name = "async_replica_client_test",
    srcs = ["async_replica_client_test.cpp"],
    tags = ["manual"],
    deps = [
        ":async_replica_client",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/common/network:mock_socket",
        "//platform/common/network:tcp_socket",
    ],
)

cc_library(
    name = "mock_async_replica_client",
    testonly = True,
    hdrs = ["mock_async_replica_client.h"],
    visibility = ["//platform:__subpackages__"],
    deps = [
        ":async_replica_client",
    ],
)

cc_library(
    name = "replica_communicator",
    srcs = ["replica_communicator.cpp"],
    hdrs = ["replica_communicator.h"],
    visibility = ["//platform:__subpackages__"],
    deps = [
        ":async_replica_client",
        "//interface/rdbc:net_channel",
        "//platform/common/queue:batch_queue",
        "//platform/proto:broadcast_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "mock_replica_communicator",
    testonly = True,
    hdrs = ["mock_replica_communicator.h"],
    visibility = ["//platform:__subpackages__"],
    deps = [
        ":replica_communicator",
    ],
)

cc_test(
    name = "replica_communicator_test",
    srcs = ["replica_communicator_test.cpp"],
    deps = [
        ":mock_async_replica_client",
        ":replica_communicator",
        "//common/test:test_main",
        "//interface/rdbc:mock_net_channel",
        "//platform/common/network:mock_socket",
    ],
)
