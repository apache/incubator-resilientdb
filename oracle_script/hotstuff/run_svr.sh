
CONFIG_PATH=$1

bazel build //application/hotstuff:kv_server_performance 
bazel run //oracle_script/hotstuff/script:run_svr $CONFIG_PATH
