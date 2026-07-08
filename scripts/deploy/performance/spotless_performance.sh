export server=//benchmark/protocols/spotless:kv_server_performance
export TEMPLATE_PATH=$PWD/config/spotless.config

./performance/run_performance.sh $*
exit 0
