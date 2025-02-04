export server=//benchmark/protocols/poe:kv_server_performance
export TEMPLATE_PATH=$PWD/config/poe.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
