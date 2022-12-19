package(default_visibility = ["//benchmark:__subpackages__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "benchmark_proto",
    srcs = ["benchmark.proto"],
)

cc_proto_library(
    name = "benchmark_cc_proto",
    deps = [":benchmark_proto"],
)
