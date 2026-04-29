export server=//benchmark/protocols/sync_fides:kv_server_performance
export TEMPLATE_PATH=$PWD/config/sync_fides.config
export ENABLE_TEE=1

./performance/run_performance.sh $*
