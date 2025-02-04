package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//service/kv_service/executor:kv_server_executor",
        "//service/utils:server_factory",
    ],
)

cc_binary(
    name = "kv_server_with_verifier",
    srcs = ["kv_server_with_verifier.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//service/kv_service/executor:kv_server_executor_with_verifier",
        "//service/utils:server_factory",
    ],
)

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/ordering/pbft:consensus_service_pbft",
        "//service/kv_service/executor:kv_server_executor",
        "//service/utils:server_factory",
    ],
)
