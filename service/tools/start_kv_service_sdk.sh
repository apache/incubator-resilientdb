killall -9 kv_server

SERVER_PATH=./bazel-bin/service/kv_service/kv_server
SERVER_CONFIG=service/tools/config/server/server.config
WORK_PATH=$PWD
CERT_PATH=${WORK_PATH}/service/tools/data/cert/

bazel build --cxxopt='-std=c++17' //service/kv_service:kv_server --action_env=PYTHON_BIN_PATH="/usr/bin/python3.8" --action_env=PYTHON_LIB_PATH="/usr/lib/python3.8"
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node1.key.pri $CERT_PATH/cert_1.cert > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node2.key.pri $CERT_PATH/cert_2.cert > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node3.key.pri $CERT_PATH/cert_3.cert > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node4.key.pri $CERT_PATH/cert_4.cert > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node5.key.pri $CERT_PATH/cert_5.cert > client.log &
