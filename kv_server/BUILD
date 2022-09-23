package(default_visibility = ["//visibility:public"])

# if enable storage layer add the flag below to bazel command
# --define=enable_my_flag=true
# example: bazel build :kv_server_executor_test --define=enable_my_flag=true

config_setting(
    name = "durable_layer",
    values = {
         "define":"enable_my_flag=true"
    }
)

cc_library(
    name = "kv_server_executor",
    srcs = ["kv_server_executor.cpp"],
    hdrs = ["kv_server_executor.h"],
    deps = [
        "//common:comm",
        "//config:resdb_config_utils",
        "//execution:transaction_executor_impl",
        "//proto:kv_server_cc_proto",
    ] +
    select({
          ":durable_layer":[
            "//durable_layer:leveldb_durable",
            "//durable_layer:rocksdb_durable"
          ],
            "//conditions:default": [],
    }),
    defines = select({
          ":durable_layer":[
            'EnableStorage'
          ],
           "//conditions:default": [],
    }),
)

cc_test(
    name = "kv_server_executor_test",
    srcs = ["kv_server_executor_test.cpp"],
    deps = [
        ":kv_server_executor",
        "//common/test:test_main",
    ],
)

cc_binary(
    name = "kv_server",
    srcs = ["kv_server.cpp"],
    deps = [
        ":kv_server_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
    ],
)

cc_binary(
    name = "kv_server_performance",
    srcs = ["kv_server_performance.cpp"],
    deps = [
        ":kv_server_executor",
        "//application/utils:server_factory",
        "//config:resdb_config_utils",
        "//ordering/pbft:consensus_service_pbft",
    ],
)
