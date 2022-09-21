killall -9 kv_server

SERVER_PATH=./bazel-bin/kv_server/kv_server
SERVER_CONFIG=example/kv_config.config
WORK_PATH=$PWD

bazel build kv_server:kv_server
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node1.key.pri $WORK_PATH/cert/cert_1.cert 127.0.0.1:8091 > server0.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node2.key.pri $WORK_PATH/cert/cert_2.cert 127.0.0.1:8092 > server1.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node3.key.pri $WORK_PATH/cert/cert_3.cert 127.0.0.1:8093 > server2.log &
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node4.key.pri $WORK_PATH/cert/cert_4.cert 127.0.0.1:8094 > server3.log &

nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/cert/node5.key.pri $WORK_PATH/cert/cert_5.cert 127.0.0.1:8095 > client.log &
