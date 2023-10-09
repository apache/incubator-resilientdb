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
        "//chain/state:chain_state",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
    ],
)

cc_library(
    name = "ext_kv_executor",
    srcs = ["ext_kv_executor.cpp"],
    hdrs = ["ext_kv_executor.h"],
    deps = [
        ":kv_executor",
        "//chain/state:chain_state",
        "//common:comm",
        "//executor/common:transaction_manager",
        "//platform/config:resdb_config_utils",
        "//proto/kv:kv_cc_proto",
    ],
)

cc_library(
    name = "quecc_executor",
    srcs = ["quecc_executor.cpp"],
    hdrs = ["quecc_executor.h"],
    deps = [
        "//chain/state:chain_state",
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
        "//chain/storage:mock_storage",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "quecc_executor_test",
    srcs = ["quecc_executor_test.cpp"],
    deps = [
        ":quecc_executor",
        "//chain/storage:mock_storage",
        "//common/test:test_main",
    ],
)
