
export WORKER_NUM=32

CONFIG_PATH=$PWD/benchmark.conf
sh $PWD/../run_svr.sh $CONFIG_PATH
