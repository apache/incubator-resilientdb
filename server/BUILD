package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_comm",
    hdrs = ["server_comm.h"],
    deps = [
        "//client:resdb_client",
        "//proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "resdb_service",
    srcs = ["resdb_service.cpp"],
    hdrs = ["resdb_service.h"],
    deps = [
        ":server_comm",
        "//common/data_comm",
        "//config:resdb_config",
    ],
)

cc_library(
    name = "mock_resdb_service",
    hdrs = ["mock_resdb_service.h"],
    deps = [
        ":resdb_service",
        "//common/test",
    ],
)

cc_library(
    name = "consensus_service",
    srcs = ["consensus_service.cpp"],
    hdrs = ["consensus_service.h"],
    deps = [
        ":resdb_replica_client",
        ":resdb_service",
        "//common:comm",
        "//common/queue:blocking_queue",
        "//config:resdb_config",
        "//proto:broadcast_cc_proto",
        "//proto:resdb_cc_proto",
        "//statistic:stats",
    ],
)

cc_test(
    name = "consensus_service_test",
    srcs = ["consensus_service_test.cpp"],
    deps = [
        ":consensus_service",
        ":mock_resdb_replica_client",
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
        "//config:resdb_config",
    ],
)

cc_test(
    name = "async_acceptor_test",
    srcs = ["async_acceptor_test.cpp"],
    deps = [
        ":async_acceptor",
        "//common/network:tcp_socket",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "resdb_server",
    srcs = ["resdb_server.cpp"],
    hdrs = ["resdb_server.h"],
    deps = [
        ":async_acceptor",
        ":resdb_service",
        "//common/data_comm",
        "//common/network:tcp_socket",
        "//common/queue:lock_free_queue",
        "//proto:broadcast_cc_proto",
        "//statistic:stats",
    ],
)

cc_test(
    name = "resdb_server_test",
    srcs = ["resdb_server_test.cpp"],
    deps = [
        ":mock_resdb_service",
        ":resdb_server",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "async_replica_client",
    srcs = ["async_replica_client.cpp"],
    hdrs = ["async_replica_client.h"],
    deps = [
        "//client:resdb_client",
        "//common:asio",
        "//common/queue:blocking_queue",
        "//common/queue:lock_free_queue",
        "//proto:broadcast_cc_proto",
    ],
)

cc_test(
    name = "async_replica_client_test",
    srcs = ["async_replica_client_test.cpp"],
    deps = [
        ":async_replica_client",
        "//client:mock_resdb_client",
        "//common/network:mock_socket",
        "//common/network:tcp_socket",
        "//common/test:test_main",
        "@boost//:thread",
    ],
)

cc_library(
    name = "mock_async_replica_client",
    testonly = True,
    hdrs = ["mock_async_replica_client.h"],
    deps = [
        ":async_replica_client",
    ],
)

cc_library(
    name = "resdb_replica_client",
    srcs = ["resdb_replica_client.cpp"],
    hdrs = ["resdb_replica_client.h"],
    deps = [
        ":async_replica_client",
        "//client:resdb_client",
        "//common/queue:batch_queue",
        "//proto:broadcast_cc_proto",
        "//statistic:stats",
    ],
)

cc_library(
    name = "mock_resdb_replica_client",
    testonly = True,
    hdrs = ["mock_resdb_replica_client.h"],
    deps = [
        ":resdb_replica_client",
    ],
)

cc_test(
    name = "resdb_replica_client_test",
    srcs = ["resdb_replica_client_test.cpp"],
    deps = [
        ":mock_async_replica_client",
        ":resdb_replica_client",
        "//client:mock_resdb_client",
        "//common/network:mock_socket",
        "//common/test:test_main",
    ],
)
