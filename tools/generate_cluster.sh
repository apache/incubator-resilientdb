
sh $PWD/tools/generate_key.sh cert/node1
sh $PWD/tools/generate_certificate.sh --node_pub_key=cert/node1.key.pub --node_id=1 --ip=127.0.0.1 --port=10001 --save_path=cert/

sh $PWD/tools/generate_key.sh cert/node2
sh $PWD/tools/generate_certificate.sh --node_pub_key=cert/node2.key.pub --node_id=2 --ip=127.0.0.1 --port=10002 --save_path=cert/

sh $PWD/tools/generate_key.sh cert/node3
sh $PWD/tools/generate_certificate.sh --node_pub_key=cert/node3.key.pub --node_id=3 --ip=127.0.0.1 --port=10003 --save_path=cert/

sh $PWD/tools/generate_key.sh cert/node4
sh $PWD/tools/generate_certificate.sh --node_pub_key=cert/node4.key.pub --node_id=4 --ip=127.0.0.1 --port=10004 --save_path=cert/
