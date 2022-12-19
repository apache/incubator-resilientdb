
CONFIG_PATH=$1

bazel build //application/tendermint:kv_server_performance 
bazel run //oracle_script/tendermint/script:run_svr $CONFIG_PATH
