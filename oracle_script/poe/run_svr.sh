
CONFIG_PATH=$1

bazel build //application/poe:kv_server_performance 
bazel run //oracle_script/poe/script:run_svr $CONFIG_PATH
