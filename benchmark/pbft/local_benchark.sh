BIN=benchmark_client_main
ROOT_PATH=$PWD/
PBFT_BM_SRC=//benchmark/pbft
PBFT_BM_BIN=${ROOT_PATH}/bazel-bin/benchmark/pbft/${BIN}

value_size=10
req_num=10
thd_num=2
client_ip=127.0.0.1

bazel build $PBFT_BM_SRC:${BIN}

$PBFT_BM_BIN ${ROOT_PATH}/benchmark/poc/pbft/benchmark_server.config  ${ROOT_PATH}/cert/node5.key.pri ${ROOT_PATH}/cert/cert_5.cert ${value_size} ${req_num} ${thd_num} ${client_ip}

