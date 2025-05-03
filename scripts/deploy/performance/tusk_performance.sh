export server=//benchmark/protocols/tusk:kv_server_performance
export TEMPLATE_PATH=$PWD/config/tusk.config

./performance/run_performance.sh $*

