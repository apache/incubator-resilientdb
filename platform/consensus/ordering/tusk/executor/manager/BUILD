package(default_visibility = ["//visibility:public"])

cc_library(
    name = "address_manager",
    srcs = ["address_manager.cpp"],
    hdrs = ["address_manager.h"],
    deps = [
        "//service/contract/executor/common:utils",
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
        "//service/contract/executor/common:utils",
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
        ":data_storage",
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
    name = "local_view",
    srcs = ["local_view.cpp"],
    hdrs = ["local_view.h"],
    deps = [
        ":concurrency_controller",
        "//common:comm",
    ],
)

cc_test(
    name = "local_view_test",
    srcs = ["local_view_test.cpp"],
    deps = [
        ":local_view",
        ":two_phase_controller",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "evm_state",
    hdrs = ["evm_state.h"],
    deps = [
        "//service/contract/executor/common:utils",
        "//common:comm",
    ],
)

cc_library(
    name = "global_state",
    srcs = ["global_state.cpp"],
    hdrs = ["global_state.h"],
    deps = [
        ":evm_state",
        "//service/contract/executor/common:utils",
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
        "//service/contract/executor/common:utils",
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
    name = "two_phase_ooo_controller",
    srcs = ["two_phase_ooo_controller.cpp"],
    hdrs = ["two_phase_ooo_controller.h"],
    deps = [
        ":concurrency_controller",
        "//common:comm",
    ],
)

cc_library(
    name = "two_phase_ooo_committer",
    srcs = ["two_phase_ooo_committer.cpp"],
    hdrs = ["two_phase_ooo_committer.h"],
    deps = [
        ":contract_committer",
        ":two_phase_ooo_controller",
        ":contract_executor",
        ":local_state",
        ":global_state",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)


cc_library(
    name = "two_phase_controller",
    srcs = ["two_phase_controller.cpp"],
    hdrs = ["two_phase_controller.h"],
    deps = [
        ":concurrency_controller",
        "//common:comm",
    ],
)

cc_test(
    name = "two_phase_controller_test",
    srcs = ["two_phase_controller_test.cpp"],
    deps = [
        ":two_phase_controller",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "ooo_controller",
    srcs = ["ooo_controller.cpp"],
    hdrs = ["ooo_controller.h"],
    deps = [
        ":concurrency_controller",
        "//common:comm",
    ],
)

cc_library(
    name = "sequential_cc_controller",
    srcs = ["sequential_cc_controller.cpp"],
    hdrs = ["sequential_cc_controller.h"],
    deps = [
        ":concurrency_controller",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
    ],
)

cc_test(
    name = "sequential_cc_controller_test",
    srcs = ["sequential_cc_controller_test.cpp"],
    deps = [
        ":sequential_cc_controller",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "streaming_dq_controller",
    srcs = ["streaming_dq_controller.cpp"],
    hdrs = ["streaming_dq_controller.h"],
    deps = [
        ":concurrency_controller",
        ":d_storage",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
    ],
)


cc_library(
    name = "test_controller",
    srcs = ["test_controller.cpp"],
    hdrs = ["test_controller.h"],
    testonly = True,
    deps = [
        ":concurrency_controller",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
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
    name = "contract_verifier",
    hdrs = ["contract_verifier.h"],
    srcs = ["contract_verifier.cpp"],
    deps = [
        ":contract_executor",
        ":contract_committer",
        ":committer_context",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "ooo_committer",
    srcs = ["ooo_committer.cpp"],
    hdrs = ["ooo_committer.h"],
    deps = [
        ":contract_committer",
        ":ooo_controller",
        ":contract_executor",
        ":local_state",
        ":global_state",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "two_phase_committer",
    srcs = ["two_phase_committer.cpp"],
    hdrs = ["two_phase_committer.h"],
    deps = [
        ":contract_committer",
        ":two_phase_controller",
        ":contract_executor",
        ":local_state",
        ":global_state",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_test(
    name = "two_phase_committer_test",
    srcs = ["two_phase_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
    ],
    deps = [
        ":contract_deployer",
        ":two_phase_committer",
        ":address_manager",
        "//common/test:test_main",
    ],
)


cc_library(
    name = "sequential_concurrency_committer",
    srcs = ["sequential_concurrency_committer.cpp"],
    hdrs = ["sequential_concurrency_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":sequential_cc_controller",
        ":local_state",
        ":global_state",
        ":committer_context",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "x_committer",
    srcs = ["x_committer.cpp"],
    hdrs = ["x_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":x_controller",
        ":streaming_dq_controller",
        ":local_state",
        ":global_state",
        ":committer_context",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "x_controller",
    srcs = ["x_controller.cpp"],
    hdrs = ["x_controller.h"],
    deps = [
        ":concurrency_controller",
        "//platform/common/queue:lock_free_queue",
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

cc_test(
    name = "x_committer_test",
    srcs = ["x_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
    ],
    deps = [
        ":contract_deployer",
        ":x_committer",
        ":address_manager",
        ":mock_data_storage",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "x_verifier_test",
    srcs = ["x_verifier_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":x_verifier",
        ":x_committer",
        ":address_manager",
        ":mock_data_storage",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "streaming_single_committer",
    srcs = ["streaming_single_committer.cpp"],
    hdrs = ["streaming_single_committer.h"],
    deps = [
        ":committer_context",
        ":contract_committer",
        ":contract_executor",
        ":global_state",
        "//common/utils:utils",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "streaming_dq_committer",
    srcs = ["streaming_dq_committer.cpp"],
    hdrs = ["streaming_dq_committer.h"],
    deps = [
        ":committer_context",
        ":contract_committer",
        ":contract_executor",
        ":streaming_dq_controller",
        ":local_state",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_test(
    name = "streaming_dq_committer_test",
    srcs = ["streaming_dq_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
        "//service/contract/executor/manager/test_data:kv.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_d_storage",
        ":streaming_dq_committer",
        "//common/test:test_main",
    ],
)


cc_test(
    name = "sequential_concurrency_committer_test",
    srcs = ["sequential_concurrency_committer_test.cpp"],
    data = [
        "//service/contract/executor/manager/test_data:contract.json",
    ],
    deps = [
        ":contract_deployer",
        ":address_manager",
        ":mock_data_storage",
        ":sequential_concurrency_committer",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "streaming_controller",
    srcs = ["streaming_controller.cpp"],
    hdrs = ["streaming_controller.h"],
    deps = [
        ":concurrency_controller",
        "//platform/common/queue:lock_free_queue",
        "//common:comm",
    ],
)

cc_library(
    name = "streaming_committer",
    srcs = ["streaming_committer.cpp"],
    hdrs = ["streaming_committer.h"],
    deps = [
        ":contract_committer",
        ":contract_executor",
        ":streaming_controller",
        ":local_state",
        ":global_state",
        "//common/utils:utils",
        "//platform/common/queue:lock_free_queue",
        "//service/contract/proto:func_params_cc_proto",
    ],
)

cc_library(
    name = "test_committer",
    hdrs = ["test_committer.h"],
    srcs = ["test_committer.cpp"],
    testonly = True,
    deps = [
        ":test_controller",
        ":contract_committer",
        ":contract_executor",
        ":global_state",
        "//service/contract/proto:func_params_cc_proto",
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
        ":two_phase_committer",
        ":sequential_concurrency_committer",
        ":ooo_committer",
        ":two_phase_ooo_committer",
        ":x_committer",
        ":x_verifier",
        ":streaming_committer",
        ":concurrency_controller",
        ":streaming_single_committer",
        ":streaming_dq_committer",
        ":global_state",
        ":address_manager",
        "//service/contract/executor/common:utils",
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

