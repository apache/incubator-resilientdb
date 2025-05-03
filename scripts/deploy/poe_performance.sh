export server=//benchmark/protocols/rcc:kv_server_performance
export TEMPLATE_PATH=$PWD/config/rcc.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
