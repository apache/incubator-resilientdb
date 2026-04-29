# sudo add-apt-repository ppa:ethereum/ethereum
# sudo apt-get update
# sudo apt-get install solc
solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize kv.sol > kv.json

