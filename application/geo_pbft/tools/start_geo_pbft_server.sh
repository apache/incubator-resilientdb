BIN=geo_pbft_server
WORK_PATH=$PWD

SERVER_SRC_PATH=application/geo_pbft
SERVER_BIN_PATH=$WORK_PATH/bazel-bin/${SERVER_SRC_PATH}/${BIN}
LOG_PATH=$WORK_PATH/log/geo_pbft
SERVER_CONFIG1=$SERVER_SRC_PATH/geo_pbft_region1.config
SERVER_CONFIG2=$SERVER_SRC_PATH/geo_pbft_region2.config
SERVER_CONFIG3=$SERVER_SRC_PATH/geo_pbft_region3.config
mkdir -p $LOG_PATH

killall -9 $BIN
bazel build ${SERVER_SRC_PATH}:${BIN}

nohup $SERVER_BIN_PATH $SERVER_CONFIG1 $WORK_PATH/cert/node_1.key.pri $WORK_PATH/cert/cert_1.cert > $LOG_PATH/server1.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG1 $WORK_PATH/cert/node_2.key.pri $WORK_PATH/cert/cert_2.cert > $LOG_PATH/server2.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG1 $WORK_PATH/cert/node_3.key.pri $WORK_PATH/cert/cert_3.cert > $LOG_PATH/server3.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG1 $WORK_PATH/cert/node_4.key.pri $WORK_PATH/cert/cert_4.cert > $LOG_PATH/server4.log &

nohup $SERVER_BIN_PATH $SERVER_CONFIG2 $WORK_PATH/cert/node_5.key.pri $WORK_PATH/cert/cert_5.cert > $LOG_PATH/server5.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG2 $WORK_PATH/cert/node_6.key.pri $WORK_PATH/cert/cert_6.cert > $LOG_PATH/server6.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG2 $WORK_PATH/cert/node_7.key.pri $WORK_PATH/cert/cert_7.cert > $LOG_PATH/server7.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG2 $WORK_PATH/cert/node_8.key.pri $WORK_PATH/cert/cert_8.cert > $LOG_PATH/server8.log &

nohup $SERVER_BIN_PATH $SERVER_CONFIG3 $WORK_PATH/cert/node_9.key.pri $WORK_PATH/cert/cert_9.cert > $LOG_PATH/server9.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG3 $WORK_PATH/cert/node_10.key.pri $WORK_PATH/cert/cert_10.cert > $LOG_PATH/server10.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG3 $WORK_PATH/cert/node_11.key.pri $WORK_PATH/cert/cert_11.cert > $LOG_PATH/server11.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG3 $WORK_PATH/cert/node_12.key.pri $WORK_PATH/cert/cert_12.cert > $LOG_PATH/server12.log &

nohup $SERVER_BIN_PATH $SERVER_CONFIG1 $WORK_PATH/cert/node_13.key.pri $WORK_PATH/cert/cert_13.cert > $LOG_PATH/client1.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG2 $WORK_PATH/cert/node_14.key.pri $WORK_PATH/cert/cert_14.cert > $LOG_PATH/client2.log &
nohup $SERVER_BIN_PATH $SERVER_CONFIG3 $WORK_PATH/cert/node_15.key.pri $WORK_PATH/cert/cert_15.cert > $LOG_PATH/client3.log &
