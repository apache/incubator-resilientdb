package(default_visibility = ["//executor/utxo:__subpackages__"])

cc_library(
    name = "tx_mempool",
    srcs = ["tx_mempool.cpp"],
    hdrs = ["tx_mempool.h"],
    deps = [
        "//common:comm",
        "//proto/utxo:utxo_cc_proto",
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
        "//common:comm",
        "//proto/utxo:utxo_cc_proto",
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
        "//common:comm",
        "//common/crypto:hash",
        "//common/crypto:signature_utils",
        "//proto/utxo:config_cc_proto",
        "//proto/utxo:utxo_cc_proto",
    ],
)

cc_test(
    name = "transaction_test",
    srcs = ["transaction_test.cpp"],
    deps = [
        ":transaction",
        "//common/crypto:key_generator",
        "//common/test:test_main",
    ],
)
