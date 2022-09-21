package(default_visibility = ["//test:__pkg__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "resdb_test_proto",
    srcs = ["resdb_test.proto"],
)

cc_proto_library(
    name = "resdb_test_cc_proto",
    deps = [":resdb_test_proto"],
)
