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
        "//application/contract/proto:func_params_cc_proto",
        "//common:comm",
    ],
)

cc_test(
    name = "contract_manager_test",
    srcs = ["contract_manager_test.cpp"],
    data = [
        "//application/contract/manager/test_data:contract.json",
    ],
    deps = [
        ":contract_manager",
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
