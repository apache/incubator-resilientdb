ID=$1
sh $PWD/tools/generate_key.sh cert/node${ID}
sh $PWD/tools/generate_certificate.sh --node_pub_key=cert/node${ID}.key.pub --node_id=$ID --save_path=cert/

