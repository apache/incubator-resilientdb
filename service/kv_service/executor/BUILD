package(default_visibility = ["//visibility:public"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

bool_flag(
    name = "enable_leveldb",
    build_setting_default = False,
    visibility = ["//visibility:private"],
)

bool_flag(
    name = "enable_rocksdb",
    build_setting_default = False,
    visibility = ["//visibility:private"],
)

config_setting(
    name = "enable_leveldb_setting",
    values = {
        "define": "enable_leveldb=True",
    },
    visibility = ["//visibility:private"],
)

config_setting(
    name = "enable_rocksdb_setting",
    values = {
        "define": "enable_rocksdb=True",
    },
    visibility = ["//visibility:private"],
)

cc_library(
    name = "kv_service_transaction_manager",
    srcs = ["kv_service_transaction_manager.cpp"],
    hdrs = ["kv_service_transaction_manager.h"],
    copts = select({
        ":enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        ":enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//common:comm",
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:transaction_manager",
        "//storage:in_mem_kv_storage",
        "//service/kv_service/proto:kv_server_cc_proto",
    ] + select({
        ":enable_leveldb_setting": ["//storage:res_leveldb"],
        ":enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "kv_service_transaction_manager_test",
    srcs = ["kv_service_transaction_manager_test.cpp"],
    deps = [
        ":kv_service_transaction_manager",
        "//common/test:test_main",
        "//storage:mock_storage",
    ],
)

