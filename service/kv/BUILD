package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_service",
    srcs = ["kv_service.cpp"],
    copts = select({
        "//chain/storage/setting:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//chain/storage/setting:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//platform/config:resdb_config_utils",
        "//executor/kv:kv_executor",
        "//service/utils:server_factory",
        "//common:comm",
        "//proto/kv:kv_cc_proto",
        "//chain/storage:memory_db",
    ] + select({
        "//chain/storage/setting:enable_leveldb_setting": ["//chain/storage:leveldb"],
        "//chain/storage/setting:enable_rocksdb_setting": ["//chain/storage:rocksdb"],
        "//conditions:default": [],
    }),
)
