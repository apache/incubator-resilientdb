package(default_visibility = ["//application/utxo:__subpackages__"])

cc_library(
    name = "tx_mempool",
    srcs = ["tx_mempool.cpp"],
    hdrs = ["tx_mempool.h"],
    deps = [
        "//application/utxo/proto:utxo_cc_proto",
        "//common:comm",
    ],
)

cc_test(
    name = "tx_mempool_test",
    srcs = ["tx_mempool_test.cpp"],
    deps = [
        ":tx_mempool",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "wallet",
    srcs = ["wallet.cpp"],
    hdrs = ["wallet.h"],
    deps = [
        "//application/utxo/proto:utxo_cc_proto",
        "//common:comm",
    ],
)

cc_test(
    name = "wallet_test",
    srcs = ["wallet_test.cpp"],
    deps = [
        ":wallet",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "transaction",
    srcs = ["transaction.cpp"],
    hdrs = ["transaction.h"],
    deps = [
        ":tx_mempool",
        ":wallet",
        "//application/utxo/proto:config_cc_proto",
        "//application/utxo/proto:utxo_cc_proto",
        "//common:comm",
        "//crypto:hash",
        "//crypto:signature_utils",
    ],
)

cc_test(
    name = "transaction_test",
    srcs = ["transaction_test.cpp"],
    deps = [
        ":transaction",
        "//common/test:test_main",
        "//crypto:key_generator",
    ],
)
