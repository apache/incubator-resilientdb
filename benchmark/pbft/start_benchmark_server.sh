WORK_PATH=$PWD

BIN=benchmark_server
SERVER_SRC_PATH=benchmark/pbft
SERVER_BIN_PATH=$WORK_PATH/bazel-bin/${SERVER_SRC_PATH}/${BIN}
LOG_PATH=$WORK_PATH/log/pbft
SERVER_CONFIG=${WORK_PATH}/$SERVER_SRC_PATH/benchmark_server.config
mkdir -p $LOG_PATH

killall -9 $BIN
bazel build //${SERVER_SRC_PATH}:${BIN}

nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node1.key.pri $WORK_PATH/cert/cert_1.cert > $LOG_PATH/server0.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node2.key.pri $WORK_PATH/cert/cert_2.cert > $LOG_PATH/server1.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node3.key.pri $WORK_PATH/cert/cert_3.cert > $LOG_PATH/server2.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node4.key.pri $WORK_PATH/cert/cert_4.cert > $LOG_PATH/server3.log &
