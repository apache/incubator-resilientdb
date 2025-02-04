package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utxo_client",
    srcs = ["utxo_client.cpp"],
    hdrs = ["utxo_client.h"],
    deps = [
        "//interface/rdbc:transaction_constructor",
        "//proto/utxo:rpc_cc_proto",
        "//proto/utxo:utxo_cc_proto",
    ],
)
