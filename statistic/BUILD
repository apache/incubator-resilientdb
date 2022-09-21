package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stats",
    srcs = ["stats.cpp"],
    hdrs = ["stats.h"],
    deps = [
        ":prometheus_handler",
        "//common:comm",
        "//common/utils",
        "//third_party:prometheus",
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
