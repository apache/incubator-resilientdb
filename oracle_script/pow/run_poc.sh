
CONFIG=$1

cp ~/nexres/oracle_script/pbft/rep_4/server.config ~/nexres/oracle_script/pow/bft_server.config

bazel build //application/poc:pow_server
bazel run //oracle_script/pow:run_svr ${CONFIG}


