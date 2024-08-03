export server=//benchmark/protocols/cassandra:kv_server_performance
export TEMPLATE_PATH=$PWD/config/cassandra.config

./performance/run_performance.sh $*
