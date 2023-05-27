package(default_visibility = ["//visibility:public"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

bool_flag(
    name = "enable_leveldb",
    build_setting_default = False,
    visibility = ["//visibility:public"],
)

bool_flag(
    name = "enable_rocksdb",
    build_setting_default = False,
    visibility = ["//visibility:public"],
)

config_setting(
    name = "enable_leveldb_setting",
    values = {
        "define": "enable_leveldb=True",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "enable_rocksdb_setting",
    values = {
        "define": "enable_rocksdb=True",
    },
    visibility = ["//visibility:public"],
)

cc_library(
    name = "kv_executor",
    srcs = ["kv_executor.cpp"],
    hdrs = ["kv_executor.h"],
    deps = [
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
        "//storage:in_mem_kv_storage",
    ],
)

cc_test(
    name = "kv_executor_test",
    srcs = ["kv_executor_test.cpp"],
    deps = [
        ":kv_executor",
        "//common/test:test_main",
        "//storage:mock_storage",
    ],
)
