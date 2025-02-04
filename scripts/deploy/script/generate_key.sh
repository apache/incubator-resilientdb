
bazel_path=$1; shift
output_path=$1; shift
key_num=$1

echo "generate key in:"${output_path}
echo "key num:"$key_num

bazel build //tools:key_generator_tools
rm -rf ${output_path}
mkdir -p ${output_path}

for idx in `seq 1 ${key_num}`;
do
  echo `${bazel_path}/bazel-bin/tools/key_generator_tools "${output_path}/node_${idx}" "AES"`
done
