export server=//benchmark/protocols/multipaxos:kv_server_performance
export TEMPLATE_PATH=$PWD/config/multipaxos.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
