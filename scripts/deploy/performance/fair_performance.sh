export server=//benchmark/protocols/fairdag:kv_server_performance
export TEMPLATE_PATH=$PWD/config/fair.config

./performance/run_performance.sh $*
