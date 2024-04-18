export server=//benchmark/protocols/poe:kv_server_performance
export TEMPLATE_PATH=$PWD/config/poe.config

./performance/run_performance.sh $*
