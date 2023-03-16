package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_proto_grpc//python:defs.bzl", "python_proto_library")

proto_library(
    name = "network_type_proto",
    srcs = ["network_type.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "network_type_cc_proto",
    deps = [":network_type_proto"],
)

proto_library(
    name = "replica_info_proto",
    srcs = ["replica_info.proto"],
    deps = [
        ":durable_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "replica_info_cc_proto",
    deps = [
        ":replica_info_proto",
    ],
)

python_proto_library(
    name = "replica_info_py_proto",
    protos = [
        ":durable_proto",
        ":replica_info_proto",
    ],
    deps = [
        "//common/proto:signature_info_py_proto",
    ],
)

proto_library(
    name = "durable_proto",
    srcs = ["durable.proto"],
)

cc_proto_library(
    name = "durable_cc_proto",
    deps = [
        ":durable_proto",
    ],
)

proto_library(
    name = "resdb_proto",
    srcs = ["resdb.proto"],
    deps = [
        ":replica_info_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "resdb_cc_proto",
    deps = [":resdb_proto"],
)

proto_library(
    name = "client_test_proto",
    srcs = ["client_test.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "client_test_cc_proto",
    deps = [":client_test_proto"],
)

proto_library(
    name = "logging_proto",
    srcs = ["logging.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "logging_cc_proto",
    deps = [":logging_proto"],
)

proto_library(
    name = "checkpoint_info_proto",
    srcs = ["checkpoint_info.proto"],
    deps = [
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "checkpoint_info_cc_proto",
    deps = [":checkpoint_info_proto"],
)

proto_library(
    name = "viewchange_message_proto",
    srcs = ["viewchange_message.proto"],
    deps = [
        ":checkpoint_info_proto",
        ":resdb_proto",
        "//common/proto:signature_info_proto",
    ],
)

cc_proto_library(
    name = "viewchange_message_cc_proto",
    deps = [":viewchange_message_proto"],
)

proto_library(
    name = "broadcast_proto",
    srcs = ["broadcast.proto"],
    deps = [
    ],
)

cc_proto_library(
    name = "broadcast_cc_proto",
    deps = [":broadcast_proto"],
)
