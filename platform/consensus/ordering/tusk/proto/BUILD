package(default_visibility = ["//platform/consensus/ordering/tusk:__pkg__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "tusk_proto",
    srcs = ["tusk.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "tusk_cc_proto",
    deps = [":tusk_proto"],
)
