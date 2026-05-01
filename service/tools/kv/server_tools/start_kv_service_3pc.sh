killall -9 kv_service kv_service_3pc 2>/dev/null || true

SERVER_PATH=./bazel-bin/service/kv/kv_service_3pc
SERVER_CONFIG=service/tools/config/server/server.config
CERT_PATH=$PWD/service/tools/data/cert

bazel build //service/kv:kv_service_3pc $@
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node1.key.pri $CERT_PATH/cert_1.cert > server0.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node2.key.pri $CERT_PATH/cert_2.cert > server1.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node3.key.pri $CERT_PATH/cert_3.cert > server2.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node4.key.pri $CERT_PATH/cert_4.cert > server3.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node5.key.pri $CERT_PATH/cert_5.cert > server4.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node6.key.pri $CERT_PATH/cert_6.cert > server5.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node7.key.pri $CERT_PATH/cert_7.cert > server6.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node8.key.pri $CERT_PATH/cert_8.cert > server7.log 2>&1 &

nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node9.key.pri $CERT_PATH/cert_9.cert > server8.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node10.key.pri $CERT_PATH/cert_10.cert > server9.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node11.key.pri $CERT_PATH/cert_11.cert > server10.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node12.key.pri $CERT_PATH/cert_12.cert > server11.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node13.key.pri $CERT_PATH/cert_13.cert > server12.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node14.key.pri $CERT_PATH/cert_14.cert > server13.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node15.key.pri $CERT_PATH/cert_15.cert > server14.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node16.key.pri $CERT_PATH/cert_16.cert > server15.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node17.key.pri $CERT_PATH/cert_17.cert > server16.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node18.key.pri $CERT_PATH/cert_18.cert > server17.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node19.key.pri $CERT_PATH/cert_19.cert > server18.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node20.key.pri $CERT_PATH/cert_20.cert > server19.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node21.key.pri $CERT_PATH/cert_21.cert > server20.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node22.key.pri $CERT_PATH/cert_22.cert > server21.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node23.key.pri $CERT_PATH/cert_23.cert > server22.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node24.key.pri $CERT_PATH/cert_24.cert > server23.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node25.key.pri $CERT_PATH/cert_25.cert > server24.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node26.key.pri $CERT_PATH/cert_26.cert > server25.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node27.key.pri $CERT_PATH/cert_27.cert > server26.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node28.key.pri $CERT_PATH/cert_28.cert > server27.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node29.key.pri $CERT_PATH/cert_29.cert > server28.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node30.key.pri $CERT_PATH/cert_30.cert > server29.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node31.key.pri $CERT_PATH/cert_31.cert > server30.log 2>&1 &
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node32.key.pri $CERT_PATH/cert_32.cert > server31.log 2>&1 &


nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node33.key.pri $CERT_PATH/cert_33.cert > client.log 2>&1 &
