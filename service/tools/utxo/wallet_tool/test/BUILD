package(default_visibility = ["//visibility:private"])

cc_binary(
    name = "key_tester_utils.so",
    srcs = ["key_tester_utils.cpp"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//common:pybind",
        "//common/crypto:signature_utils",
    ],
)

py_library(
    name = "key_tester_utils_so",
    data = [":key_tester_utils.so"],
    imports = ["."],
)

py_binary(
    name = "key_tester",
    srcs = ["key_tester.py"],
    legacy_create_init = False,
    python_version = "PY3",
    deps = [
        ":key_tester_utils_so",
        "//service/tools/utxo/wallet_tool/pybind:wallet_tools_py_so",
    ],
)
