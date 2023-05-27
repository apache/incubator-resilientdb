killall -9 kv_server

SERVER_PATH=./bazel-bin/server/kv/kv_server
SERVER_CONFIG=server/tools/config/server/server.config
WORK_PATH=$PWD
CERT_PATH=${WORK_PATH}/server/tools/data/cert/

bazel build //server/kv:kv_server $@
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node1.key.pri $CERT_PATH/cert_1.cert > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node2.key.pri $CERT_PATH/cert_2.cert > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node3.key.pri $CERT_PATH/cert_3.cert > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node4.key.pri $CERT_PATH/cert_4.cert > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node5.key.pri $CERT_PATH/cert_5.cert > client.log &
