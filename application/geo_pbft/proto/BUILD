package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "geo_pbft_proto",
    srcs = ["geo_pbft.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "geo_pbft_cc_proto",
    deps = [":geo_pbft_proto"],
)
