
config=$1
output_path=$2

. ${config}

echo "config file:"${config}
echo "generate key in:"${output_path}

bazel build //tools:key_generator_tools
rm -rf ${output_path}/cert/
mkdir -p ${output_path}/cert/

for idx in `seq 1 ${#iplist[@]}`;
do
  echo `${BAZEL_WORKSPACE_PATH}/bazel-bin/tools/key_generator_tools "${output_path}/cert/node_${idx}" "AES"`
done
