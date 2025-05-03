killall -9 kv_service

SERVER_PATH=./bazel-bin/service/kv/kv_service
SERVER_CONFIG=service/tools/config/server/server.config
WORK_PATH=$PWD
CERT_PATH=${WORK_PATH}/service/tools/data/cert/
GRAFANA_PORT=8090

bazel build //service/kv:kv_service $@
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node1.key.pri $CERT_PATH/cert_1.cert 8090 > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node2.key.pri $CERT_PATH/cert_2.cert 8091 > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node3.key.pri $CERT_PATH/cert_3.cert 8092 > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node4.key.pri $CERT_PATH/cert_4.cert 8093 > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node5.key.pri $CERT_PATH/cert_5.cert 8094 > client.log &
