package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "benchmark_client",
    srcs = ["benchmark_client.cpp"],
    tags = ["manual"],
    deps = [
        "//application/poc:pbft_client",
        "//config:resdb_config_utils",
    ],
)
