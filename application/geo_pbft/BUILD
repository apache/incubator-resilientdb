package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "geo_pbft_server",
    srcs = ["geo_pbft_server.cpp"],
    deps = [
        ":geo_pbft_executor_impl",
        "//config:resdb_config_utils",
        "//ordering/geo_pbft:consensus_service_geo_pbft",
        "//server:resdb_server",
    ],
)

cc_library(
    name = "geo_pbft_executor_impl",
    srcs = ["geo_pbft_executor_impl.cpp"],
    hdrs = ["geo_pbft_executor_impl.h"],
    deps = [
        "//application/geo_pbft/proto:geo_pbft_cc_proto",
        "//common:comm",
        "//execution:transaction_executor_impl",
    ],
)

cc_library(
    name = "geo_pbft_client",
    srcs = ["geo_pbft_client.cpp"],
    hdrs = ["geo_pbft_client.h"],
    deps = [
        "//application/geo_pbft/proto:geo_pbft_cc_proto",
        "//client:resdb_user_client",
    ],
)

cc_binary(
    name = "geo_pbft_client_main",
    srcs = ["geo_pbft_client_main.cpp"],
    deps = [
        ":geo_pbft_client",
        "//config:resdb_config_utils",
    ],
)

cc_binary(
    name = "geo_pbft_server_performance",
    srcs = ["geo_pbft_server_performance.cpp"],
    deps = [
        ":geo_pbft_executor_impl",
        "//config:resdb_config_utils",
        "//kv_server:kv_server_executor",
        "//ordering/geo_pbft:consensus_service_geo_pbft",
        "//server:resdb_server",
        "//statistic:stats",
    ],
)

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//application/kv_server_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
        "//ordering/geo_pbft:consensus_service_geo_pbft",
    ],
)
