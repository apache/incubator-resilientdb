package(default_visibility = ["//platform/consensus/ordering/poe_mac:__pkg__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "poe_proto",
    srcs = ["poe.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "poe_cc_proto",
    deps = [":poe_proto"],
)
