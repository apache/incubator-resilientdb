!/bin/bash

killall -9 kv_server

WORK_PATH=$1
WORK_SPACE=${WORK_PATH}/../../
SERVER_PATH=${WORK_SPACE}/bazel-bin/kv_server/kv_server
SERVER_CONFIG=${WORK_PATH}/deploy/server.config
LOG_PATH=${WORK_PATH}/deploy/log

cd ${WORK_PATH}

bazel build //kv_server:kv_server
mkdir -p ${LOG_PATH} 

SVR_NUM=$2

echo "server num:"${SVR_NUM}
METRIC_PORT=8091
for i in `seq 1 ${SVR_NUM}`;
do
echo ${i}
nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/deploy/cert/node_${i}.key.pri $WORK_PATH/deploy/cert/cert_${i}.cert 0.0.0.0:${METRIC_PORT} > ${LOG_PATH}/server${i}.log 2>&1 &
METRIC_PORT=`expr ${METRIC_PORT} + 1`
done

SVR_NUM=`expr ${SVR_NUM} + 1`
echo ${SVR_NUM}

nohup $SERVER_PATH $SERVER_CONFIG $WORK_PATH/deploy/cert/node_${SVR_NUM}.key.pri $WORK_PATH/deploy/cert/cert_${SVR_NUM}.cert 0.0.0.0:${METRIC_PORT} > ${LOG_PATH}/client.log 2>&1 &
