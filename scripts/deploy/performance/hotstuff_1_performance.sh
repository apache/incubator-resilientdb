export server=//benchmark/protocols/hotstuff_1:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hotstuff_1.config

./performance/run_performance.sh $*
exit 0
