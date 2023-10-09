package(default_visibility = ["//visibility:private"])

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

cc_binary(
    name = "kv_service",
    srcs = ["kv_service.cpp"],
    copts = select({
        "//executor/kv:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//executor/kv:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//platform/config:resdb_config_utils",
        "//executor/kv:kv_executor",
        "//service/utils:server_factory",
        "//common:comm",
        "//chain/state:chain_state",
        "//proto/kv:kv_cc_proto",
    ] + select({
        "//executor/kv:enable_leveldb_setting": ["//storage:res_leveldb"],
        "//executor/kv:enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)

cc_binary(
    name = "quecc_kv_service",
    srcs = ["quecc_kv_service.cpp"],
    copts = select({
        "//executor/kv:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//executor/kv:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//platform/config:resdb_config_utils",
        "//executor/kv:quecc_executor",
        "//service/utils:server_factory",
        "//common:comm",
        "//chain/state:chain_state",
        "//proto/kv:kv_cc_proto",
    ] + select({
        "//executor/kv:enable_leveldb_setting": ["//storage:res_leveldb"],
        "//executor/kv:enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)

cc_binary(
    name = "ext_kv_service",
    srcs = ["kv_service.cpp"],
    copts = select({
        "//executor/kv:enable_leveldb_setting": ["-DENABLE_LEVELDB"],
        "//executor/kv:enable_rocksdb_setting": ["-DENABLE_ROCKSDB"],
        "//conditions:default": [],
    }),
    deps = [
        "//platform/config:resdb_config_utils",
        "//executor/kv:ext_kv_executor",
        "//service/utils:server_factory",
        "//common:comm",
        "//chain/state:chain_state",
        "//proto/kv:kv_cc_proto",
    ] + select({
        "//executor/kv:enable_leveldb_setting": ["//storage:res_leveldb"],
        "//executor/kv:enable_rocksdb_setting": ["//storage:res_rocksdb"],
        "//conditions:default": [],
    }),
)
