package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//chain/storage:memory_db",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//service/utils:server_factory",
    ],
)

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "//common/proto:signature_info_cc_proto",
        "//interface/kv:kv_client",
        "//platform/config:resdb_config_utils",
    ],
)
