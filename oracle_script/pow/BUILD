package(default_visibility = ["//visibility:public"])

py_binary(
    name = "run_svr",
    srcs = ["run_svr.py"],
    deps = [
        "//oracle_script/comm:comm_config",
        "//oracle_script/comm:utils",
    ],
)

py_binary(
    name = "run_cli",
    srcs = ["run_cli.py"],
    deps = [
        "//oracle_script/comm:comm_config",
        "//oracle_script/comm:utils",
    ],
)

py_binary(
    name = "benchmark_result",
    srcs = ["benchmark_result.py"],
    deps = [
        "//oracle_script/comm:comm_config",
        "//oracle_script/comm:utils",
    ],
)
