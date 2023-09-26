package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "kv_server_proto",
    srcs = ["kv_server.proto"],
)

cc_proto_library(
    name = "kv_server_cc_proto",
    deps = [":kv_server_proto"],
)
