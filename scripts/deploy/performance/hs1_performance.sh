export server=//benchmark/protocols/hs1:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hs1.config

./performance/run_performance.sh $*
