package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    hdrs = ["utils.h"],
    deps = [
        "//third_party:evm_lib",
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
    name = "data_storage",
    srcs = ["data_storage.cpp"],
    hdrs = ["data_storage.h"],
    deps = [
        ":utils",
        "//common:comm",
    ],
)

cc_library(
    name = "mock_data_storage",
    hdrs = ["mock_data_storage.h"],
    deps = [ 
        ":storage",
        "//common/test:test"
    ],
)

cc_library(
    name = "mock_d_storage",
    hdrs = ["mock_d_storage.h"],
    deps = [ 
        ":d_storage",
        "//common/test:test"
    ],
)


cc_library(
    name = "d_storage",
    srcs = ["d_storage.cpp"],
    hdrs = ["d_storage.h"],
    deps = [
        ":data_storage",
        "//common:comm",
    ],
)


cc_library(
    name = "leveldb",
    srcs = ["leveldb.cpp"],
    hdrs = ["leveldb.h"],
    deps = [
        ":data_storage",
        "//common:comm",
        "//storage:res_leveldb"
    ],
)

cc_library(
    name = "global_view",
    srcs = ["global_view.cpp"],
    hdrs = ["global_view.h"],
    deps = [
        ":data_storage",
        "//common:comm",
    ],
)

cc_library(
    name = "db_view",
    srcs = ["db_view.cpp"],
    hdrs = ["db_view.h"],
    deps = [
        ":concurrency_controller",
        ":xcontroller",
        "//common:comm",
    ],
)

cc_library(
    name = "executor_state",
    srcs = ["executor_state.cpp"],
    hdrs = ["executor_state.h"],
    deps = [
        ":evm_state",
        ":utils",
        ":db_view",
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
        ":utils",
        ":global_view",
        "//common:comm",
    ],
)

cc_library(
    name = "local_state",
    srcs = ["local_state.cpp"],
    hdrs = ["local_state.h"],
    deps = [
        ":evm_state",
        ":utils",
        ":local_view",
        "//common:comm",
    ],
)

cc_library(
    name = "concurrency_controller",
    srcs = ["concurrency_controller.cpp"],
    hdrs = ["concurrency_controller.h"],
    deps = [
        ":data_storage",
    ],
)

cc_library(
    name = "xcontroller",
    srcs = ["xcontroller.cpp"],
    hdrs = ["xcontroller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)

cc_library(
    name = "mock_e_controller",
    hdrs = ["mock_e_controller.h"],
    deps = [ 
        ":xcontroller",
        "//common/test:test"
    ],
)

cc_library(
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    deps = [
        ":evm_state",
        "//common:comm",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "committer_context",
    srcs = ["committer_context.cpp"],
    hdrs = ["committer_context.h"],
    deps = [
      ":contract_committer",
    ],
)

cc_library(
    name = "contract_committer",
    hdrs = ["contract_committer.h"],
    deps = [
        ":contract_executor",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "local_view",
    srcs = ["local_view.cpp"],
    hdrs = ["local_view.h"],
    deps = [
        ":concurrency_controller",
        "//common:comm",
    ],
)


cc_library(
    name = "xexecutor",
    srcs = ["xexecutor.cpp"],
    hdrs = ["xexecutor.h"],
    deps = [
      ":local_state",
        ":contract_committer",
        ":contract_executor",
        ":committer_context",
        ":xcontroller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_test(
    name = "xexecutor_test",
    srcs = ["xexecutor_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_d_storage",
        ":mock_e_controller",
        ":xexecutor",
        "//common/test:test_main",
    ],
)


cc_library(
    name = "contract_deployer",
    srcs = ["contract_deployer.cpp"],
    hdrs = ["contract_deployer.h"],
    deps = [
        ":concurrency_controller",
        ":contract_committer",
        ":global_state",
        ":address_manager",
    ],
)

cc_test(
    name = "contract_deployer_test",
    srcs = ["contract_deployer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
    ],
    deps = [
        ":test_committer",
        ":contract_deployer",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "contract_manager",
    srcs = ["contract_manager.cpp"],
    hdrs = ["contract_manager.h"],
    deps = [
        ":contract_deployer",
        ":xexecutor",
        ":global_state",
        ":address_manager",
        ":utils",
        "//common:comm",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_test(
    name = "contract_manager_test",
    srcs = ["contract_manager_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
    ],
    deps = [
        ":contract_manager",
        "//common/test:test_main",
    ],
)

