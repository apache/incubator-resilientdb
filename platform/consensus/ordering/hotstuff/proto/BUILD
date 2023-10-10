package(default_visibility = ["//platform/consensus/ordering/hotstuff:__pkg__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")

proto_library(
    name = "hotstuff_proto",
    srcs = ["hotstuff.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "hotstuff_cc_proto",
    deps = [":hotstuff_proto"],
)
