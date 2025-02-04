export server=//benchmark/protocols/zzy:kv_server_performance
export TEMPLATE_PATH=$PWD/config/zzy.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
