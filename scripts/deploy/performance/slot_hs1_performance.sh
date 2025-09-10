export server=//benchmark/protocols/slot_hs1:kv_server_performance
export TEMPLATE_PATH=$PWD/config/slot_hs1.config

./performance/run_performance.sh $*
