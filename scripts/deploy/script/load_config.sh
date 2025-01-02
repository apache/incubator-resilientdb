KEY_FILE="config/key.conf"
. $1

if [ ! -f "${KEY_FILE}" ]; then
"please create \"${KEY_FILE}\" and put your ssh key in it"
exit -1
fi
. ${KEY_FILE}
