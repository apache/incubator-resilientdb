
CONFIG_PATH=$1

bazel build //kv_server:kv_server_performance
bazel run //oracle_script/pbft/script:run_svr $CONFIG_PATH
