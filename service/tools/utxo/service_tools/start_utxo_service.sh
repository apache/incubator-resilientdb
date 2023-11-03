killall -9 utxo_server

SERVER_PATH=./bazel-bin/service/utxo/utxo_service
SERVER_CONFIG=service/tools/config/server/server.config
WORK_PATH=$PWD
CERT_PATH=${WORK_PATH}/service/tools/data/cert/
UTXO_CONFIG=service/tools/config/server/utxo_config.config


bazel build //service/utxo:utxo_service
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node1.key.pri $CERT_PATH/cert_1.cert ${UTXO_CONFIG} > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node2.key.pri $CERT_PATH/cert_2.cert ${UTXO_CONFIG} > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node3.key.pri $CERT_PATH/cert_3.cert ${UTXO_CONFIG} > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node4.key.pri $CERT_PATH/cert_4.cert ${UTXO_CONFIG} > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node5.key.pri $CERT_PATH/cert_5.cert ${UTXO_CONFIG} > client.log &
