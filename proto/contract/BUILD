package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "func_params_proto",
    srcs = ["func_params.proto"],
)

cc_proto_library(
    name = "func_params_cc_proto",
    deps = [":func_params_proto"],
)

proto_library(
    name = "account_proto",
    srcs = ["account.proto"],
)

cc_proto_library(
    name = "account_cc_proto",
    deps = [":account_proto"],
)

proto_library(
    name = "contract_proto",
    srcs = ["contract.proto"],
)

cc_proto_library(
    name = "contract_cc_proto",
    deps = [":contract_proto"],
)

proto_library(
    name = "rpc_proto",
    srcs = ["rpc.proto"],
    deps = [
        ":account_proto",
        ":contract_proto",
        ":func_params_proto",
    ],
)

cc_proto_library(
    name = "rpc_cc_proto",
    deps = [":rpc_proto"],
)
