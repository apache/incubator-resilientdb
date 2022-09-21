package(default_visibility = ["//visibility:public"])

cc_library(
    name = "absl",
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "asio",
    deps = [
        "@boost//:asio",
    ],
)

cc_library(
    name = "boost_lockfree",
    deps = [
        "@boost//:lockfree",
    ],
)

cc_library(
    name = "glog",
    deps = [
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "comm",
    deps = [
        ":absl",
        ":glog",
    ],
)
