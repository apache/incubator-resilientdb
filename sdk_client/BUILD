cc_library(
    name = "sdk_transaction",
    srcs = ["sdk_transaction.cpp"],
    hdrs = ["sdk_transaction.h"],
    deps = [
        "//third_party:rapidjson",
    ],
)

cc_library(
    name = "crow_service",
    srcs = ["crow_service.cpp"],
    hdrs = ["crow_service.h"],
    deps = [
        ":sdk_transaction",
        "//client:resdb_txn_client",
        "//common:comm",
        "//config:resdb_config_utils",
        "//kv_client:resdb_kv_client",
        "//proto:signature_info_cc_proto",
        "//third_party:crow",
        "//third_party:rapidjson",
    ],
)

cc_binary(
    name = "crow_service_main",
    srcs = ["crow_service_main.cpp"],
    deps = [
        ":crow_service",
    ],
)

cc_test(
    name = "crow_service_test",
    srcs = ["crow_service_test.cpp"],
    deps = [
        ":crow_service",
        "//common/test:test_main",
    ],
)
