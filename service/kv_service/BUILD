package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_kv_client",
    srcs = ["resdb_kv_client.cpp"],
    hdrs = ["resdb_kv_client.h"],
    deps = [
        "@com_resdb_nexres//interface/rdbc:transaction_constructor",
        "//service/kv_service/proto:kv_server_cc_proto",
    ],
)

cc_library(
    name = "py_verificator",
    srcs = ["py_verificator.cpp"],
    hdrs = ["py_verificator.h"],
    tags = ["manual"],
    deps = [
        "@com_resdb_nexres//common:comm",
        "@pybind11//:pybind11_embed"
    ],
)

cc_library(
    name = "kv_service_transaction_manager",
    srcs = ["kv_service_transaction_manager.cpp"],
    hdrs = ["kv_service_transaction_manager.h"],
    deps = [
        ":py_verificator",
        "@com_resdb_nexres//common:comm",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "@com_resdb_nexres//executor/common:transaction_manager",
        "@com_resdb_nexres//chain/state:chain_state",
        "//service/kv_service/proto:kv_server_cc_proto",
    ]
)

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        ":kv_service_transaction_manager",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "@com_resdb_nexres//service/utils:server_factory",
    ],
)

cc_binary(
    name = "kv_service_tools",
    srcs = ["kv_service_tools.cpp"],
    deps = [
        "@com_resdb_nexres//common/proto:signature_info_cc_proto",
        "@com_resdb_nexres//platform/config:resdb_config_utils",
        "//service/kv_service:resdb_kv_client",
    ],
)


