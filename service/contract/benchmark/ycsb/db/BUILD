package(default_visibility = ["//service/contract/benchmark/ycsb:__subpackages__"])

cc_library(
    name = "contract_db",
    srcs = ["contract_db.cpp"],
    hdrs = ["contract_db.h"],
    deps = [
        ":contract_executor",
        "//common:comm",
        "//common/utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/executor/common:contract_execute_info",
        "//third_party:ycsb_lib",
    ],
)

cc_library(
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    data = [
        "//service/contract/benchmark/data:smallbank.json",
        "//service/contract/benchmark/data:ycsb.json",
    ],
    deps = [
        "//common:comm",
        "//common:json",
        "//common/utils",
        "//service/contract/benchmark/executor:benchmark_executor",
        "//service/contract/executor/common:contract_execute_info",
        "//service/contract/executor/common:utils",
        "//service/contract/proto:func_params_cc_proto",
        "//third_party:evm_lib",
    ],
)
