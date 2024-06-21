export server=//benchmark/protocols/fairdag_rl:kv_server_performance
export TEMPLATE_PATH=$PWD/config/fairrl.config

./performance/run_performance.sh $*
