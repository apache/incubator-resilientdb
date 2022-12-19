
export WORKER_NUM=16
export MAX_PROCESS_TXN=1024

CONFIG_PATH=$PWD/benchmark.conf
sh $PWD/../run_svr.sh $CONFIG_PATH
