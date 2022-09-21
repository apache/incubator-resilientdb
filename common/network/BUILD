package(default_visibility = ["//visibility:public"])

cc_library(
    name = "socket",
    hdrs = ["socket.h"],
    deps = [
        "//common:comm",
    ],
)

cc_library(
    name = "mock_socket",
    hdrs = ["mock_socket.h"],
    deps = [
        ":socket",
        "//common:comm",
        "//common/test",
    ],
)

cc_library(
    name = "tcp_socket",
    srcs = ["tcp_socket.cpp"],
    hdrs = ["tcp_socket.h"],
    deps = [
        ":socket",
    ],
)

cc_test(
    name = "tcp_socket_test",
    srcs = ["tcp_socket_test.cpp"],
    deps = [
        ":tcp_socket",
        "//common/test:test_main",
    ],
)
