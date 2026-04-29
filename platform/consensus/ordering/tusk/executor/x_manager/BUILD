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
    name = "leveldb_storage",
    srcs = ["leveldb_storage.cpp"],
    hdrs = ["leveldb_storage.h"],
    deps = [
        ":data_storage",
        "//storage:res_leveldb",
        "//common:comm",
    ],
)



cc_library(
    name = "mock_data_storage",
    hdrs = ["mock_data_storage.h"],
    deps = [ 
        ":data_storage",
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
    name = "leveldb_d_storage",
    srcs = ["leveldb_d_storage.cpp"],
    hdrs = ["leveldb_d_storage.h"],
    deps = [
        ":d_storage",
        "//storage:res_leveldb",
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
        ":streaming_e_controller",
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
    name = "concurrency_controller",
    srcs = ["concurrency_controller.cpp"],
    hdrs = ["concurrency_controller.h"],
    deps = [
        ":data_storage",
    ],
)

cc_library(
    name = "streaming_e_controller",
    srcs = ["streaming_e_controller.cpp"],
    hdrs = ["streaming_e_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)

cc_library(
    name = "e_controller",
    srcs = ["e_controller.cpp"],
    hdrs = ["e_controller.h"],
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
        ":streaming_e_controller",
        "//common/test:test"
    ],
)

cc_library(
    name = "x_controller",
    srcs = ["x_controller.cpp"],
    hdrs = ["x_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)

cc_library(
    name = "fx_controller",
    srcs = ["fx_controller.cpp"],
    hdrs = ["fx_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)

cc_library(
    name = "dx_controller",
    srcs = ["dx_controller.cpp"],
    hdrs = ["dx_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)


cc_library(
    name = "contract_executor",
    srcs = ["contract_executor.cpp"],
    hdrs = ["contract_executor.h"],
    deps = [
        ":evm_state",
        ":concurrency_controller",
        "//common:comm",
        "//service/contract/proto:func_params_cc_proto",
        "//service/contract/executor/common:contract_execute_info",
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
        "//service/contract/executor/common:contract_execute_info",
    ],
)

cc_library(
    name = "streaming_e_committer",
    srcs = ["streaming_e_committer.cpp"],
    hdrs = ["streaming_e_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":streaming_e_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        #"//platform/common/queue:priority_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "e_committer",
    srcs = ["e_committer.cpp"],
    hdrs = ["e_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":e_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "seq_committer",
    srcs = ["seq_committer.cpp"],
    hdrs = ["seq_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)


cc_test(
    name = "e_committer_test",
    srcs = ["e_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_data_storage",
        ":e_committer",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "2pl_committer",
    srcs = ["2pl_committer.cpp"],
    hdrs = ["2pl_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":2pl_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "2pl_controller",
    srcs = ["2pl_controller.cpp"],
    hdrs = ["2pl_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
        "//common/utils:utils",
    ],
)

cc_library(
    name = "x_committer",
    srcs = ["x_committer.cpp"],
    hdrs = ["x_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":x_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "fx_committer",
    srcs = ["fx_committer.cpp"],
    hdrs = ["fx_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":fx_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "dx_committer",
    srcs = ["dx_committer.cpp"],
    hdrs = ["dx_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":executor_state",
        ":committer_context",
        ":dx_controller",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_test(
    name = "x_committer_test",
    srcs = ["x_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_data_storage",
        ":x_committer",
        "//common/test:test_main",
    ],
)


cc_test(
    name = "streaming_e_committer_test",
    srcs = ["streaming_e_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_d_storage",
        ":mock_e_controller",
        ":streaming_e_committer",
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
        ":e_committer",
        ":x_committer",
        ":dx_committer",
        ":seq_committer",
        ":fx_committer",
        ":streaming_e_committer",
        ":2pl_committer",
        ":x_verifier",
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

cc_library(
    name = "contract_verifier",
    hdrs = ["contract_verifier.h"],
    srcs = ["contract_verifier.cpp"],
    deps = [
        ":contract_executor",
        ":contract_committer",
        ":committer_context",
        ":concurrency_controller",
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
    name = "v_controller",
    srcs = ["v_controller.cpp"],
    hdrs = ["v_controller.h"],
    deps = [
        ":concurrency_controller",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
    ],
)

cc_library(
    name = "x_verifier",
    srcs = ["x_verifier.cpp"],
    hdrs = ["x_verifier.h"],
    deps = [
        ":contract_verifier",
        ":local_state",
        ":global_state",
        ":v_controller",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)
