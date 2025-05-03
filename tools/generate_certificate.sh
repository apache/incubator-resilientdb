bazel build tools/certificate_tools

CERT_PATH=cert/
ADMIN_PRIVATE_KEY_PATH=cert/admin.key.pri
ADMIN_PUBLIC_KEY_PATH=cert/admin.key.pub
NODE_PUBLIC_KEY_PATH=
NODE_ID=
IP=
PORT=

options=$(getopt -l "save_path:,node_pub_key:,node_id:,ip:,port:" -o "" -u -- "$@")
#options=$(getopt -l "version:" -o "" -a -- "$@")

usage() { 
        echo "$0 usage: --node_pub_key : the path of the public key  " 
}

set -- $options

while [ $# -gt 0 ]
do
    case $1 in
    --save_path) CERT_PATH="$2"; shift;;
    --node_pub_key) NODE_PUBLIC_KEY_PATH="$2"; shift ;;
    --node_id) NODE_ID="$2"; shift ;;
    --ip) IP="$2"; shift ;;
    --port) PORT="$2"; shift ;;
    --)
    shift
    break;;
    esac
    shift
done

echo "bazel-bin/tools/certificate_tools $CERT_PATH $ADMIN_PRIVATE_KEY_PATH $ADMIN_PUBLIC_KEY_PATH $NODE_PUBLIC_KEY_PATH $NODE_ID $IP $PORT"
bazel-bin/tools/certificate_tools $CERT_PATH $ADMIN_PRIVATE_KEY_PATH $ADMIN_PUBLIC_KEY_PATH $NODE_PUBLIC_KEY_PATH $NODE_ID $IP $PORT

