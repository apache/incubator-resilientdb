package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "geo_pbft_server",
    srcs = ["geo_pbft_server.cpp"],
    deps = [
        "//platform/communication:resdb_server",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/geo_pbft:consensus_service_geo_pbft",
        "//service/kv_service/executor:kv_server_executor",
    ],
)

cc_binary(
    name = "geo_pbft_server_performance",
    srcs = ["geo_pbft_server_performance.cpp"],
    deps = [
        "//platform/communication:resdb_server",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/geo_pbft:consensus_service_geo_pbft",
        "//platform/statistic:stats",
        "//service/kv_service/executor:kv_server_executor",
    ],
)
