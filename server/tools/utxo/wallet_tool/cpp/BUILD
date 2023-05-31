package(default_visibility = ["//server/tools/utxo/wallet_tool/pybind:__subpackages__"])

cc_library(
    name = "addr_utils",
    srcs = ["addr_utils.cpp"],
    hdrs = ["addr_utils.h"],
    deps = [
        "//common/crypto:hash",
    ],
)

cc_library(
    name = "key_utils",
    srcs = ["key_utils.cpp"],
    hdrs = ["key_utils.h"],
    deps = [
        "//common/crypto:key_generator",
    ],
)

cc_binary(
    name = "utxo_client_tools",
    srcs = ["utxo_client_tools.cpp"],
    deps = [
        "//common/crypto:signature_utils",
        "//interface/utxo:utxo_client",
        "//platform/config:resdb_config_utils",
    ],
)
