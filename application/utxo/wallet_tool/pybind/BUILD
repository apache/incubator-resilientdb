package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "wallet_tools_py.so",
    srcs = ["wallet_tools_py.cpp"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//application/utxo/wallet_tool/cpp:addr_utils",
        "//application/utxo/wallet_tool/cpp:key_utils",
        "@pybind11",
    ],
)

py_library(
    name = "wallet_tools_py_so",
    data = [":wallet_tools_py.so"],
    imports = ["."],
)
