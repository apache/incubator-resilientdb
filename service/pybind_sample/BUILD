load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

cc_binary(
    name = "client",
    srcs = ["client.cpp"],
    deps = [
        "@pybind11",
        "@pybind11//:pybind11_embed",
    ],
)
