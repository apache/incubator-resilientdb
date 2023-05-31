package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_manager_pbft",
        "//server/utils:server_factory",
    ],
)
