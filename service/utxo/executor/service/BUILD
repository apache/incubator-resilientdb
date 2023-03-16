package(default_visibility = ["//service/utxo/server:__pkg__"])

cc_library(
    name = "utxo_transaction_manager",
    srcs = ["utxo_transaction_manager.cpp"],
    hdrs = ["utxo_transaction_manager.h"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/consensus/execution:custom_query",
        "//platform/consensus/execution:transaction_manager",
        "//service/utxo/executor/manager:transaction",
        "//service/utxo/executor/manager:wallet",
        "//service/utxo/proto:config_cc_proto",
        "//service/utxo/proto:rpc_cc_proto",
    ],
)

cc_test(
    name = "utxo_transaction_manager_test",
    srcs = ["utxo_transaction_manager_test.cpp"],
    deps = [
        ":utxo_transaction_manager",
        "//common/crypto:hash",
        "//common/crypto:key_generator",
        "//common/crypto:signature_utils",
        "//common/test:test_main",
    ],
)
