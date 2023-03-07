package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

filegroup(
    name = "test_data",
    srcs = ["encode_file"],
)

proto_library(
    name = "address_proto",
    srcs = ["address.proto"],
)

cc_proto_library(
    name = "address_cc_proto",
    deps = [":address_proto"],
)

cc_binary(
    name = "test_decode",
    srcs = ["test_decode.cpp"],
    data = [
	    ":test_data",
    ],
    deps = [
	":address_cc_proto",
	"//common:comm",
    ],
)
