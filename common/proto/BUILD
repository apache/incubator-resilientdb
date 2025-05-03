package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc//python:defs.bzl", "python_proto_library")

proto_library(
    name = "signature_info_proto",
    srcs = ["signature_info.proto"],
)

cc_proto_library(
    name = "signature_info_cc_proto",
    deps = [":signature_info_proto"],
)

python_proto_library(
    name = "signature_info_py_proto",
    protos = [":signature_info_proto"],
)
