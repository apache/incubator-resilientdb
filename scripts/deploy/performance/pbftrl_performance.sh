export server=//benchmark/protocols/pbft_rl:kv_server_performance
export TEMPLATE_PATH=$PWD/config/pbft_rl.config

./performance/run_performance.sh $*
