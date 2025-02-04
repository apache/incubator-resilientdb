
set +e

CURRENT_PATH=$PWD

i=0
while [ ! -f "WORKSPACE" ]
do
cd ..
((i++))
if [ "$PWD" = "/home" ]; then
  break
fi
done

BAZEL_WORKSPACE_PATH=$PWD
if [ "$PWD" = "/home" ]; then
echo "bazel path not found"
BAZEL_WORKSPACE_PATH=$CURRENT_PATH
fi

export BAZEL_WORKSPACE_PATH=$PWD

echo "use bazel path:"$BAZEL_WORKSPACE_PATH

# go back to the current dir
cd $CURRENT_PATH
