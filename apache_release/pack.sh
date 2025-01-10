set -x

TAG_NAME=master
VERSION=v1.10.1
PREFIX=apache-resilientdb-1.10.1-rc0-incubating
PREFIX_FOLDER=apache-resilientdb-1.10.1-rc0/

PACK_PATH=$PWD/packages

cd ..
git archive --format=tar ${TAG_NAME} --prefix=${PREFIX_FOLDER} | gzip > ${PACK_PATH}/${PREFIX}-src.tar.gz

cd ${PACK_PATH}
gpg -u junchao@apache.org --armor --output ${PREFIX}-src.tar.gz.asc --detach-sign ${PREFIX}-src.tar.gz

#verify
gpg --verify ${PREFIX}-src.tar.gz.asc ${PREFIX}-src.tar.gz

#checksum
shasum -a 512 ${PREFIX}-src.tar.gz > ${PREFIX}-src.tar.gz.sha512
cat ${PREFIX}-src.tar.gz.sha512

#check checksum
shasum --check ${PREFIX}-src.tar.gz.sha512



