export server=//benchmark/protocols/pbft:kv_server_performance
export TEMPLATE_PATH=$PWD/config/pbft.config

./performance/run_performance.sh $*
echo $0
