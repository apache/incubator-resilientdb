#!/usr/bin/env bash
function symlink_to_dir() {
if [[ -z "$1" ]]; then
echo "arg 1 to symlink_to_dir is unexpectedly empty"
exit 1
fi
if [[ -z "$2" ]]; then
echo "arg 2 to symlink_to_dir is unexpectedly empty"
exit 1
fi
local target="$2"
mkdir -p "$target"
if [[ -f "$1" ]]; then
# In order to be able to use `replace_in_files`, we ensure that we create copies of specfieid
# files so updating them is possible.
if [[ "$1" == *.pc || "$1" == *.la || "$1" == *-config || "$1" == *.mk || "$1" == *.cmake ]]; then
dest="$target/$(basename "$1")"
cp "$1" "$dest" && chmod +w "$dest" && touch -r "$1" "$dest"
else
ln -s -f "$1" "$target"
fi
elif [[ -L "$1" && ! -d "$1" ]]; then
cp -pR "$1" "$2"
elif [[ -d "$1" ]]; then
SAVEIFS=$IFS
IFS=$'
'
local children=($(find "$1/" -maxdepth 1 -mindepth 1))
IFS=$SAVEIFS
local dirname=$(basename "$1")
mkdir -p "$target/$dirname"
for child in "${children[@]:-}"; do
if [[ -n "$child" && "$dirname" != *.ext_build_deps ]]; then
symlink_to_dir "$child" "$target/$dirname"
fi
done
else
echo "Can not copy $1"
fi
}
function children_to_path() {
if [ -d $EXT_BUILD_DEPS/bin ]; then
local tools=$(find "$EXT_BUILD_DEPS/bin/" -maxdepth 1 -mindepth 1)
for tool in $tools;
do
if  [[ -d "$tool" ]] || [[ -L "$tool" ]]; then
export PATH=$PATH:$tool
fi
done
fi
}
function symlink_contents_to_dir() {
if [[ -z "$1" ]]; then
echo "arg 1 to symlink_contents_to_dir is unexpectedly empty"
exit 1
fi
if [[ -z "$2" ]]; then
echo "arg 2 to symlink_contents_to_dir is unexpectedly empty"
exit 1
fi
local target="$2"
mkdir -p "$target"
if [[ -f "$1" ]]; then
symlink_to_dir "$1" "$target"
elif [[ -L "$1" && ! -d "$1" ]]; then
local actual=$(readlink "$1")
symlink_contents_to_dir "$actual" "$target"
elif [[ -d "$1" ]]; then
SAVEIFS=$IFS
IFS=$'
'
local children=($(find "$1/" -maxdepth 1 -mindepth 1))
IFS=$SAVEIFS
for child in "${children[@]:-}"; do
symlink_to_dir "$child" "$target"
done
fi
}
function replace_in_files() {
if [ -d "$1" ]; then
SAVEIFS=$IFS
IFS=$'
'
# Find all real files. Symlinks are assumed to be relative to something within the directory we're seaching and thus ignored
local files=$(find -P -f "$1" \( -type f -and \( -name "*.pc" -or -name "*.la" -or -name "*-config" -or -name "*.mk" -or -name "*.cmake" \) \))
IFS=$SAVEIFS
for file in ${files[@]}; do
local backup=$(mktemp)
touch -r "${file}" "${backup}"
sed -i '' -e 's@'"$2"'@'"$3"'@g' "${file}"
if [[ "$?" -ne "0" ]]; then
exit 1
fi
touch -r "${backup}" "${file}"
rm "${backup}"
done
fi
}
echo """"
echo ""Bazel external C/C++ Rules. Building library 'cryptopp'""
echo """"
set -euo pipefail
export EXT_BUILD_ROOT=$(pwd)
export INSTALLDIR=$EXT_BUILD_ROOT/bazel-out/darwin_arm64-fastbuild/bin/cryptopp
export BUILD_TMPDIR=$INSTALLDIR.build_tmpdir
export EXT_BUILD_DEPS=$INSTALLDIR.ext_build_deps
export PATH="$EXT_BUILD_ROOT:$PATH"
rm -rf $BUILD_TMPDIR
rm -rf $EXT_BUILD_DEPS
mkdir -p $INSTALLDIR
mkdir -p $BUILD_TMPDIR
mkdir -p $EXT_BUILD_DEPS
echo ""Environment:______________""
env
echo ""__________________________""
mkdir -p $EXT_BUILD_DEPS/bin
symlink_to_dir $EXT_BUILD_ROOT/bazel-out/darwin_arm64-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains $EXT_BUILD_DEPS/bin/
children_to_path $EXT_BUILD_DEPS/bin
export PATH="$EXT_BUILD_DEPS/bin:$PATH"
cd $BUILD_TMPDIR
replace_in_files $EXT_BUILD_DEPS \${EXT_BUILD_DEPS} $EXT_BUILD_DEPS
replace_in_files $EXT_BUILD_DEPS \${EXT_BUILD_ROOT} $EXT_BUILD_ROOT
symlink_contents_to_dir $EXT_BUILD_ROOT/external/cryptopp $BUILD_TMPDIR
set -x
ARFLAGS="-static -s" ASFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3" CFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3" CXXFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -std=c++0x -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3 -std=c++17" LDFLAGS="-undefined dynamic_lookup -headerpad_max_install_names -lc++ -lm -Wl -Wc++11-narrowing -Wunused-command-line-argument" AR="/usr/bin/libtool" CC="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" CXX="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" CPPFLAGS="" $EXT_BUILD_ROOT/bazel-out/darwin_arm64-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make/bin/make -C $BUILD_TMPDIR   PREFIX=$INSTALLDIR
ARFLAGS="-static -s" ASFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3" CFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3" CXXFLAGS="-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object -fcolor-diagnostics -fno-omit-frame-pointer -std=c++0x -no-canonical-prefixes -Wno-builtin-macro-redefined -D__DATE__="redacted" -D__TIMESTAMP__="redacted" -D__TIME__="redacted" -O3 -std=c++17" LDFLAGS="-undefined dynamic_lookup -headerpad_max_install_names -lc++ -lm -Wl -Wc++11-narrowing -Wunused-command-line-argument" AR="/usr/bin/libtool" CC="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" CXX="$EXT_BUILD_ROOT/external/local_config_cc/cc_wrapper.sh" CPPFLAGS="" $EXT_BUILD_ROOT/bazel-out/darwin_arm64-opt-exec-2B5CBBC6/bin/external/rules_foreign_cc/toolchains/make/bin/make -C $BUILD_TMPDIR install  PREFIX=$INSTALLDIR
set +x
replace_in_files $INSTALLDIR $BUILD_TMPDIR \${EXT_BUILD_DEPS}
replace_in_files $INSTALLDIR $EXT_BUILD_DEPS \${EXT_BUILD_DEPS}
replace_in_files $INSTALLDIR $EXT_BUILD_ROOT \${EXT_BUILD_ROOT}
mkdir -p $EXT_BUILD_ROOT/bazel-out/darwin_arm64-fastbuild/bin/copy_cryptopp/cryptopp
if [[ -d "$INSTALLDIR" ]]; then
  cp -L -R "$INSTALLDIR"/* "$EXT_BUILD_ROOT/bazel-out/darwin_arm64-fastbuild/bin/copy_cryptopp/cryptopp"
else
  cp -L -R "$INSTALLDIR" "$EXT_BUILD_ROOT/bazel-out/darwin_arm64-fastbuild/bin/copy_cryptopp/cryptopp"
fi
find "$EXT_BUILD_ROOT/bazel-out/darwin_arm64-fastbuild/bin/copy_cryptopp/cryptopp" -type f -exec touch -r "$INSTALLDIR" "{}" \;

cd $EXT_BUILD_ROOT
if [[ -L "bazel-out/darwin_arm64-fastbuild/bin/cryptopp/lib/libcryptopp.a" ]]; then
  target="$(python -c "import os; print(os.path.realpath('bazel-out/darwin_arm64-fastbuild/bin/cryptopp/lib/libcryptopp.a'))")"
  rm "bazel-out/darwin_arm64-fastbuild/bin/cryptopp/lib/libcryptopp.a" && cp -a "${target}" "bazel-out/darwin_arm64-fastbuild/bin/cryptopp/lib/libcryptopp.a"
fi

