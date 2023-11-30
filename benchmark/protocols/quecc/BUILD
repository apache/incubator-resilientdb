package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "quecc_benchmark",
    srcs = ["quecc_benchmark.cpp"],
    deps = [
				"@benchmark//:benchmark",
        "//chain/state:chain_state",
        "//executor/kv:kv_executor",
        "//executor/kv:quecc_executor",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
        "//chain/storage:memory_db",
    ],
)
