package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "utxo_proto",
    srcs = ["utxo.proto"],
)

cc_proto_library(
    name = "utxo_cc_proto",
    deps = [":utxo_proto"],
)

proto_library(
    name = "config_proto",
    srcs = ["config.proto"],
    deps = ["utxo_proto"],
)

cc_proto_library(
    name = "config_cc_proto",
    deps = [":config_proto"],
)

proto_library(
    name = "rpc_proto",
    srcs = ["rpc.proto"],
    deps = ["utxo_proto"],
)

cc_proto_library(
    name = "rpc_cc_proto",
    deps = [":rpc_proto"],
)
