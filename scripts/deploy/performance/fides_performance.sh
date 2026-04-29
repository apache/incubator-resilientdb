export server=//benchmark/protocols/fides:kv_server_performance
export TEMPLATE_PATH=$PWD/config/fides.config
export ENABLE_TEE=1

./performance/run_performance.sh $*

