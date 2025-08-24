export server=//benchmark/protocols/zzy:kv_server_performance
export TEMPLATE_PATH=$PWD/config/zzy.config

./performance/run_performance.sh $*
