package(default_visibility = ["//visibility:public"])

cc_library(
    name = "learner_lib",
    srcs = [
        "learner.cpp",
    ],
    hdrs = ["learner.h"],
    deps = [
        "//common:json",
        "//platform/common/network:tcp_socket",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_binary(
    name = "learner",
    srcs = ["learner_main.cpp"],
    deps = [
        ":learner_lib",
    ],
)
