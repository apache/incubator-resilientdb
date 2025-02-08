package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    hdrs = ["utils.h"],
    deps = [
        "//third_party:evm_lib",
    ],
)

cc_library(
    name = "contract_manager",
    srcs = ["contract_manager.cpp"],
    hdrs = ["contract_manager.h"],
    deps = [
        ":address_manager",
        ":utils",
        ":global_state",
        "//common:comm",
        "//proto/contract:func_params_cc_proto",
    ],
)

cc_test(
    name = "contract_manager_test",
    srcs = ["contract_manager_test.cpp"],
    data = [
        "//executor/contract/manager/test_data:contract.json",
    ],
    tags = ["manual"],
    deps = [
        ":contract_manager",
        "//chain/storage:memory_db",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "address_manager",
    srcs = ["address_manager.cpp"],
    hdrs = ["address_manager.h"],
    deps = [
        ":utils",
        "//common:comm",
    ],
)

cc_test(
    name = "address_manager_test",
    srcs = ["address_manager_test.cpp"],
    deps = [
        ":address_manager",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "global_view",
    srcs = ["global_view.cpp"],
    hdrs = ["global_view.h"],
    deps = [
        ":utils",
        "//chain/storage:storage",
        "//common:comm",
    ],
)


cc_library(
    name = "evm_state",
    hdrs = ["evm_state.h"],
    deps = [
        ":utils",
        "//common:comm",
    ],
)


cc_library(
    name = "global_state",
    srcs = ["global_state.cpp"],
    hdrs = ["global_state.h"],
    deps = [
        ":evm_state",
        ":global_view",
        "//common:comm",
    ],
)

