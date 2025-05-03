package(default_visibility = ["//platform:__subpackages__"])

cc_library(
    name = "acceptor",
    srcs = ["acceptor.cpp"],
    hdrs = ["acceptor.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//platform/common/data_comm",
        "//platform/common/data_comm:network_comm",
        "//platform/common/network:tcp_socket",
        "//platform/common/queue:lock_free_queue",
        "//platform/networkstrate:server_comm",
        "//platform/statistic:stats",
    ],
)
