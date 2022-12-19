#!/bin/sh

for idx in `seq 120 140`;
do
  echo `../bazel-bin/tools/key_generator_tools "./cert/node_${idx}" "AES"`
done
