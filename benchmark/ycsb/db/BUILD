package(default_visibility = ["//benchmark/ycsb:__subpackages__"])

cc_library(
    name = "kv_db",
    hdrs = ["kv_db.h"],
    srcs = ["kv_db.cpp"],
    deps = [
      "//third_party:ycsb_lib",
      "//common:comm",
      "//platform/common/queue:lock_free_queue",
        "//common/utils:utils",
      "//executor/kv:kv_executor",
    ],
)