export server=//benchmark/protocols/cassandra_cft:kv_server_performance
export TEMPLATE_PATH=$PWD/config/cassandra.config

./performance/run_performance.sh $*
echo $0
