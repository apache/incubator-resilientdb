package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "kv_proto",
    srcs = ["kv.proto"],
)

cc_proto_library(
    name = "kv_cc_proto",
    deps = [":kv_proto"],
)
