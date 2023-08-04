package(default_visibility = ["//platform/consensus/ordering/tendermint:__pkg__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "tendermint_proto",
    srcs = ["tendermint.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "tendermint_cc_proto",
    deps = [":tendermint_proto"],
)
