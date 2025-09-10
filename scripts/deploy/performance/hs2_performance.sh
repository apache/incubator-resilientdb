export server=//benchmark/protocols/hs2:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hs2.config

./performance/run_performance.sh $*
