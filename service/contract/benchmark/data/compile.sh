# sudo add-apt-repository ppa:ethereum/ethereum
# sudo apt-get update
# sudo apt-get install solc
#solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize smallbank.sol > smallbank.json

#solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize ERC20.sol > ERC20.json

solc --evm-version homestead --combined-json bin,hashes --pretty-json --optimize tpcc.sol > tpcc.json
