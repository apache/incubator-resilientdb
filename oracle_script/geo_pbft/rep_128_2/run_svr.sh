
CONFIG_PATH=$PWD/benchmark.conf

. $CONFIG_PATH
bazel build $svr_build_path 

cd ..
bazel run script:run_svr $CONFIG_PATH
