package(default_visibility = ["//visibility:public"])

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
        "//service/kv_service:resdb_kv_client",
        "@com_resdb_nexres//common:asio",
        "@com_resdb_nexres//common:boost_comm",
        "@com_resdb_nexres//common/proto:signature_info_cc_proto",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "@com_resdb_nexres//interface/common:resdb_txn_accessor",
	"@com_resdb_nexres//interface/common:resdb_state_accessor",
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
        "@com_resdb_nexres//common/test:test_main",
    ],
)
