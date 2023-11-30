package(default_visibility = ["//visibility:public"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_library(
    name = "kv_executor",
    srcs = ["kv_executor.cpp"],
    hdrs = ["kv_executor.h"],
    deps = [
        "//chain/storage",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
    ],
)

cc_test(
    name = "kv_executor_test",
    srcs = ["kv_executor_test.cpp"],
    deps = [
        ":kv_executor",
        "//chain/storage:memory_db",
        "//common/test:test_main",
    ],
)
