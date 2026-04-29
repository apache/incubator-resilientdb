export server=//benchmark/protocols/damysus:kv_server_performance
export TEMPLATE_PATH=$PWD/config/damysus.config
export ENABLE_TEE=1

./performance/run_performance.sh $*
