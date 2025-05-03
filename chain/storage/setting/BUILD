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
