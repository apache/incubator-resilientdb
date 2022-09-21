package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "transaction_proto",
    srcs = ["transaction.proto"],
    deps = [
        "//proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "transaction_cc_proto",
    deps = [":transaction_proto"],
)

proto_library(
    name = "pow_proto",
    srcs = ["pow.proto"],
    deps = [":transaction_proto"],
)

cc_proto_library(
    name = "pow_cc_proto",
    deps = [":pow_proto"],
)
