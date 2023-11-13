package(default_visibility = [
    "//platform:__subpackages__",
    "//service:__subpackages__",
])

cc_library(
    name = "stats",
    srcs = ["stats.cpp"],
    hdrs = ["stats.h"],
    deps = [
        "//proto/kv:kv_cc_proto",
        "//platform/proto:resdb_cc_proto",
        "//common:json",
        ":prometheus_handler",
        "//common:comm",
        "//common/utils",
        "//third_party:prometheus",
        "//platform/common/network:tcp_socket",
        "//common:asio",
        "//common:beast"
    ],
)

cc_library(
    name = "prometheus_handler",
    srcs = ["prometheus_handler.cpp"],
    hdrs = ["prometheus_handler.h"],
    deps = [
        "//common:comm",
        "//third_party:prometheus",
    ],
)

cc_binary(
    name = "set_random_data",
    srcs = ["set_random_data.cpp"],
)
