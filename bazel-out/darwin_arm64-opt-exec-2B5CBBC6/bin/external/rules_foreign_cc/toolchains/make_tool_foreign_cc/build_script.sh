#!/usr/bin/env bash
export EXT_BUILD_ROOT=$(pwd)
export INSTALLDIR=$EXT_BUILD_ROOT//make_tool
export BUILD_TMPDIR=$INSTALLDIR.build_tmpdir
export EXT_BUILD_DEPS=$INSTALLDIR.ext_build_deps
set -euo pipefail
export EXT_BUILD_ROOT=$(pwd)
export INSTALLDIR=$EXT_BUILD_ROOT/bazel-out/darwin_arm64-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make
export BUILD_TMPDIR=$INSTALLDIR.build_tmpdir
rm -rf $INSTALLDIR
rm -rf $BUILD_TMPDIR
mkdir -p $INSTALLDIR
mkdir -p $BUILD_TMPDIR
if [[ -d "./external/gnumake_src" ]]; then
  cp -L -R "./external/gnumake_src"/* "$BUILD_TMPDIR"
else
  cp -L -R "./external/gnumake_src" "$BUILD_TMPDIR"
fi
find "$BUILD_TMPDIR" -type f -exec touch -r "./external/gnumake_src" "{}" \;

cd $BUILD_TMPDIR
set -x
AR="/usr/bin/libtool" ARFLAGS="-static -s -o" CC="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" CFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -g0 -O2 -D_FORTIFY_SOURCE=1 -DNDEBUG -ffunction-sections -fdata-sections -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted"" LD="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" LDFLAGS="-undefined dynamic_lookup -headerpad_max_install_names -lc++ -lm" ./configure --without-guile --with-guile=no --disable-dependency-tracking --prefix=$INSTALLDIR
cat build.cfg
./build.sh
./make install
set +x
