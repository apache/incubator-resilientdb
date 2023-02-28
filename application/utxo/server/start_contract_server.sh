killall -9 utxo_server

SERVER_PATH=./bazel-bin/application/utxo/server/utxo_server
SERVER_CONFIG=application/utxo/server/config/server_config.config
UTXO_CONFIG=application/utxo/server/config/utxo_config.config
WORK_PATH=$PWD

bazel build //application/utxo/server:utxo_server
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node1.key.pri $WORK_PATH/cert/cert_1.cert ${UTXO_CONFIG} > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node2.key.pri $WORK_PATH/cert/cert_2.cert ${UTXO_CONFIG} > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node3.key.pri $WORK_PATH/cert/cert_3.cert ${UTXO_CONFIG} > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node4.key.pri $WORK_PATH/cert/cert_4.cert ${UTXO_CONFIG} > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node5.key.pri $WORK_PATH/cert/cert_5.cert ${UTXO_CONFIG} > client.log &
