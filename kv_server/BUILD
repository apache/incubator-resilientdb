package(default_visibility = ["//visibility:public"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

bool_flag(
    name = "enable_sdk",
    build_setting_default = False,
    visibility = ["//visibility:private"],
)

config_setting(
    name = "custom",
    values = {"define": "enable_sdk=True"},
    visibility = ["//visibility:private"],
)

cc_library(
    name = "py_verificator",
    srcs = ["py_verificator.cpp"],
    hdrs = ["py_verificator.h"],
    deps = [
        "//common:comm",
        "@pybind11",
        "@pybind11//:pybind11_embed",
    ],
)

cc_test(
    name = "py_verificator_test",
    srcs = ["py_verificator_test.cpp"],
    tags = ["manual"],
    deps = [
        ":py_verificator",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "kv_server_executor",
    srcs = ["kv_server_executor.cpp"],
    hdrs = ["kv_server_executor.h"],
    copts = select({
        ":custom": ["-DENABLED_SDK"],
        "//conditions:default": [],
    }),
    deps = [
        "//common:comm",
        "//config:resdb_config_utils",
        "//durable_layer:leveldb_durable",
        "//durable_layer:rocksdb_durable",
        "//execution:transaction_executor_impl",
        "//proto:kv_server_cc_proto",
    ] + select({
        ":custom": [":py_verificator"],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "kv_server_executor_test",
    srcs = ["kv_server_executor_test.cpp"],
    deps = [
        ":kv_server_executor",
        "//common/test:test_main",
    ],
)

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        ":kv_server_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
    ],
)

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        ":kv_server_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
        "//ordering/pbft:consensus_service_pbft",
    ],
)
