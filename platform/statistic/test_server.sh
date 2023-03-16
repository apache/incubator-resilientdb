killall -9 kv_server

SERVER_PATH=./bazel-bin/kv_server/kv_server
SERVER_CONFIG=example/kv_config.config
WORK_PATH=$PWD

bazel build kv_server:kv_server
$SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node1.key.pri $WORK_PATH/cert/cert_1.cert 127.0.0.1:8091 > server0.log &