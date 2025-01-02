export server=//benchmark/protocols/fides:kv_server_performance
export TEMPLATE_PATH=$PWD/config/fides.config

./performance/run_performance.sh $*

