package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tusk_service",
    srcs = ["tusk_service.cpp"],
    hdrs = ["tusk_service.h"],
    deps = [
        ":contract_transaction_manager",
        ":execution_helper",
        "//platform/consensus/ordering/tusk/framework:consensus",
        "//service/contract/executor/x_manager:contract_manager",
        "//service/contract/executor/x_manager:leveldb_storage",
        "//service/contract/proto:rpc_cc_proto",
    ],
)

cc_library(
    name = "execution_helper",
    srcs = ["execution_helper.cpp"],
    hdrs = ["execution_helper.h"],
    deps = [
        "//service/contract/executor/x_manager:contract_manager",
        "//service/contract/proto:rpc_cc_proto",
    ],
)

cc_library(
    name = "contract_transaction_manager",
    srcs = ["contract_transaction_manager.cpp"],
    hdrs = ["contract_transaction_manager.h"],
    deps = [
        ":execution_helper",
        "//executor/common:transaction_manager",
        "//service/contract/executor/x_manager:contract_manager",
        "//service/contract/executor/x_manager:leveldb_storage",
        "//service/contract/executor/x_manager:streaming_committer",
        "//service/contract/proto:rpc_cc_proto",
    ],
)

cc_binary(
    name = "contract_server",
    srcs = ["contract_server.cpp"],
    deps = [
        ":tusk_service",
        "//platform/config:resdb_config_utils",
        "//service/contract/benchmark/generator:string_generator",
        "//service/contract/benchmark/generator:zipfian_generator",
        "//service/contract/executor/x_manager:leveldb_storage",
        "//service/contract/tusk_service:contract_transaction_manager",
        "//service/utils:server_factory",
    ],
)
