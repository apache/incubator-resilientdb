package(default_visibility = ["//application/utxo/wallet_tool/pybind:__subpackages__"])

cc_library(
    name = "addr_utils",
    srcs = ["addr_utils.cpp"],
    hdrs = ["addr_utils.h"],
    deps = [
        "//crypto:hash",
    ],
)

cc_library(
    name = "key_utils",
    srcs = ["key_utils.cpp"],
    hdrs = ["key_utils.h"],
    deps = [
        "//crypto:key_generator",
    ],
)

cc_binary(
    name = "utxo_client_tools",
    srcs = ["utxo_client_tools.cpp"],
    deps = [
        "//application/utxo/client:utxo_client",
        "//config:resdb_config_utils",
        "//crypto:signature_utils",
    ],
)
