export server=//benchmark/protocols/achilles:kv_server_performance
export TEMPLATE_PATH=$PWD/config/achilles.config
export ENABLE_TEE=1

./performance/run_performance.sh $*
