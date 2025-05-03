export server=//benchmark/protocols/simple_pbft:kv_server_performance
export TEMPLATE_PATH=$PWD/config/simple_pbft.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
