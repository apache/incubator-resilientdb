export server=//benchmark/protocols/autobahn:kv_server_performance
export TEMPLATE_PATH=$PWD/config/autobahn.config

./performance/run_performance.sh $*
echo $0
