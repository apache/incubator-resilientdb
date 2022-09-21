#!/usr/bin/env bash
function cleanup_function() {
local ecode=$?
if [ $ecode -eq 0 ]; then
cleanup_on_success
else
cleanup_on_failure
fi
}
set -e
function cleanup_on_success() {
  rm -rf $BUILD_TMPDIR $EXT_BUILD_DEPS
}
function cleanup_on_failure() {
echo ""rules_foreign_cc: Build failed!""
echo ""rules_foreign_cc: Keeping temp build directory $BUILD_TMPDIR and dependencies directory $EXT_BUILD_DEPS for debug.""
echo ""rules_foreign_cc: Please note that the directories inside a sandbox are still cleaned unless you specify '--sandbox_debug' Bazel command line flag.""
echo ""rules_foreign_cc: Printing build logs:""
echo ""_____ BEGIN BUILD LOGS _____""
cat "$BUILD_LOG"
echo ""_____ END BUILD LOGS _____""
echo ""rules_foreign_cc: Build wrapper script location: $BUILD_WRAPPER_SCRIPT""
echo ""rules_foreign_cc: Build script location: $BUILD_SCRIPT""
echo ""rules_foreign_cc: Build log location: $BUILD_LOG""
echo """"
}
trap "cleanup_function" EXIT
export BUILD_WRAPPER_SCRIPT="bazel-out/darwin_arm64-fastbuild/bin/cryptopp_foreign_cc/wrapper_build_script.sh"
export BUILD_SCRIPT="bazel-out/darwin_arm64-fastbuild/bin/cryptopp_foreign_cc/build_script.sh"
export BUILD_LOG="bazel-out/darwin_arm64-fastbuild/bin/cryptopp_foreign_cc/Make.log"
touch $BUILD_LOG
$BUILD_SCRIPT &> $BUILD_LOG
