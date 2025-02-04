package(default_visibility = ["//platform/consensus/ordering/tusk:__subpackages__"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc//python:defs.bzl", "python_proto_library")

proto_library(
    name = "proposal_proto",
    srcs = ["proposal.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "proposal_cc_proto",
    deps = [":proposal_proto"],
)
