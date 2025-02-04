BIN=pow_server
WORK_PATH=$PWD

SERVER_SRC_PATH=application/poc
SERVER_BIN_PATH=$WORK_PATH/bazel-bin/${SERVER_SRC_PATH}/${BIN}
CERT_PATH=$WORK_PATH/$SERVER_SRC_PATH/tools/pow_cert
LOG_PATH=$WORK_PATH/log/poc/pow
BFT_CONFIG_PATH=$SERVER_SRC_PATH/pbft.config
POW_CONFIG_PATH=$SERVER_SRC_PATH/pow.config

set -x
mkdir -p $LOG_PATH

killall -9 $BIN

bazel build ${SERVER_SRC_PATH}:${BIN}
nohup $SERVER_BIN_PATH $BFT_CONFIG_PATH $POW_CONFIG_PATH $WORK_PATH/cert/node1.key.pri $CERT_PATH/cert_1.cert > $LOG_PATH/server0.log &
nohup $SERVER_BIN_PATH $BFT_CONFIG_PATH $POW_CONFIG_PATH $WORK_PATH/cert/node2.key.pri $CERT_PATH/cert_2.cert > $LOG_PATH/server1.log &
nohup $SERVER_BIN_PATH $BFT_CONFIG_PATH $POW_CONFIG_PATH $WORK_PATH/cert/node3.key.pri $CERT_PATH/cert_3.cert > $LOG_PATH/server2.log &
nohup $SERVER_BIN_PATH $BFT_CONFIG_PATH $POW_CONFIG_PATH $WORK_PATH/cert/node4.key.pri $CERT_PATH/cert_4.cert > $LOG_PATH/server3.log &
