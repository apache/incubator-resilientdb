export server=//benchmark/protocols/pompe:kv_server_performance
export TEMPLATE_PATH=$PWD/config/pompe.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
