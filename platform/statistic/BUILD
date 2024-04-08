package(default_visibility = [
    "//platform:__subpackages__",
    "//service:__subpackages__",
])

cc_library(
    name = "cpu_info",
    srcs = ["cpu_info.cpp"],
    hdrs = ["cpu_info.h"],
    deps = [
        "//common:comm",
        "//common/utils",
    ],
)

cc_test(
    name = "cpu_info_test",
    srcs = ["cpu_info_test.cpp"],
    deps = [
      ":cpu_info",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "stats",
    srcs = ["stats.cpp"],
    hdrs = ["stats.h"],
    deps = [
        ":prometheus_handler",
        ":cpu_info",
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
