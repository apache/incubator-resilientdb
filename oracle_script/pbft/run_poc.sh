
CONFIG_PATH=$1

bazel build //application/poc:pbft_server
bazel run //oracle_script/pbft/script:run_svr $CONFIG_PATH
