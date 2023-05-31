package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    copts = select({
        "//executor/kv:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//executor/kv:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//platform/config:resdb_config_utils",
        "//executor/kv:kv_executor",
        "//server/utils:server_factory",
        "//common:comm",
        "//storage:in_mem_kv_storage",
        "//proto/kv:kv_cc_proto",
    ] + select({
        "//executor/kv:enable_leveldb_setting": ["//storage:res_leveldb"],
        "//executor/kv:enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)
