BIN=pbft_server
WORK_PATH=$PWD

SERVER_SRC_PATH=application/poc
SERVER_BIN_PATH=$WORK_PATH/bazel-bin/${SERVER_SRC_PATH}/${BIN}
LOG_PATH=$WORK_PATH/log/pbft
SERVER_CONFIG=$SERVER_SRC_PATH/pbft.config
mkdir -p $LOG_PATH

killall -9 $BIN
bazel build ${SERVER_SRC_PATH}:${BIN}

nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node1.key.pri $WORK_PATH/cert/cert_1.cert > $LOG_PATH/server0.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node2.key.pri $WORK_PATH/cert/cert_2.cert > $LOG_PATH/server1.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node3.key.pri $WORK_PATH/cert/cert_3.cert > $LOG_PATH/server2.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node4.key.pri $WORK_PATH/cert/cert_4.cert > $LOG_PATH/server3.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG $WORK_PATH/cert/node5.key.pri $WORK_PATH/cert/cert_5.cert > $LOG_PATH/client.log &
