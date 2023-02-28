package(default_visibility = ["//visibility:public"])

py_binary(
    name = "keys",
    srcs = ["keys.py"],
    legacy_create_init = False,
    python_version = "PY3",
    deps = [
        "//application/utxo/wallet_tool/pybind:wallet_tools_py_so",  # a py_library
    ],
)

py_binary(
    name = "addr",
    srcs = ["addr.py"],
    legacy_create_init = False,
    python_version = "PY3",
    deps = [
        "//application/utxo/wallet_tool/pybind:wallet_tools_py_so",  # a py_library
    ],
)
