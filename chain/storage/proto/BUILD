package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "kv_proto",
    srcs = ["kv.proto"],
    visibility = ["//chain/storage:__subpackages__"],
)

cc_proto_library(
    name = "kv_cc_proto",
    visibility = ["//chain/storage:__subpackages__"],
    deps = [":kv_proto"],
)

proto_library(
    name = "leveldb_config_proto",
    srcs = ["leveldb_config.proto"],
)

cc_proto_library(
    name = "leveldb_config_cc_proto",
    deps = [":leveldb_config_proto"],
)

proto_library(
    name = "rocksdb_config_proto",
    srcs = ["rocksdb_config.proto"],
)

cc_proto_library(
    name = "rocksdb_config_cc_proto",
    deps = [":rocksdb_config_proto"],
)
