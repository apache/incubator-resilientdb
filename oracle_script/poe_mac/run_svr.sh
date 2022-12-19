
CONFIG_PATH=$1

bazel build //application/poe_mac:kv_server_performance 
bazel run //oracle_script/poe_mac/script:run_svr $CONFIG_PATH
