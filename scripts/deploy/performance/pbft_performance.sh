export server=//benchmark/protocols/pbft:kv_server_performance
export TEMPLATE_PATH=$PWD/config/template.config
#export COPTS="--define enable_leveldb=True"
#export COPTS="-pg"

./performance/run_performance.sh $*
