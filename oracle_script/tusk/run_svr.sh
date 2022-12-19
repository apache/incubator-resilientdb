
#set -o pipefail
set -e

CONFIG_PATH=$1

bazel build //application/tusk:kv_server_performance 
bazel run //oracle_script/tusk/script:run_svr $CONFIG_PATH
