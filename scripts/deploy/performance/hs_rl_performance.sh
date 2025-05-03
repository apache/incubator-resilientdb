export server=//benchmark/protocols/hs_rl:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hs_rl.config

./performance/run_performance.sh $*
