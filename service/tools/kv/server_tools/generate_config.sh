iplist=(
127.0.0.1
127.0.0.1
127.0.0.1
127.0.0.1
127.0.0.1
)

WORKSPACE=$PWD
CERT_PATH=$PWD/service/tools/data/cert/
CONFIG_PATH=$PWD/service/tools/config/
PORT_BASE=20000
CLIENT_NUM=1

./service/tools/config/generate_config.sh ${WORKSPACE} ${CERT_PATH} ${CERT_PATH} ${CONFIG_PATH} ${CERT_PATH} ${CLIENT_NUM} ${PORT_BASE} ${iplist[@]} 
