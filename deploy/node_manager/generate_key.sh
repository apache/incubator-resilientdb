
WORK_PATH=$1
WORK_SPACE=${WORK_PATH}/../../

cd "${WORK_PATH}"

NUM=`cat deploy/iplist.txt| awk '{if (length($0) > 0) {print $0}}' | wc -l`

echo "server num:$NUM"

mkdir -p deploy/cert
cd deploy
bazel build //tools:key_generator_tools

for idx in `seq 1 ${NUM}`;
do
  echo `${WORK_SPACE}/bazel-bin/tools/key_generator_tools "./cert/node_${idx}" "AES"`
done
