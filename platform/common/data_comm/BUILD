package(default_visibility = ["//visibility:public"])

cc_library(
    name = "data_comm",
    hdrs = [
        "data_comm.h",
    ],
)

cc_library(
    name = "network_comm",
    hdrs = ["network_comm.h"],
    deps = [
        ":data_comm",
        "//platform/common/network:tcp_socket",
    ],
)
