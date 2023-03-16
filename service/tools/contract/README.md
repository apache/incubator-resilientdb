start the server:

	./service/tools/contract/service_tools/start_contract_service.sh

Now build the contract tool to help you access the server:

	bazel build service/tools/contract/api_tools/contract_tools

Using the contract tools to create an account first:

	bazel-bin/service/tools/contract/api_tools/contract_tools create -c service/tools/config/interface/service.config

Deploy a contract:

	bazel-bin/service/tools/contract/api_tools/contract_tools deploy -c service/tools/config/interface/service.config -p service/tools/contract/api_tools/example_contract/token.json -n token.sol:Token -a 1000 -m 0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8

Transfer:

	bazel-bin/service/tools/contract/api_tools/contract_tools execute -c service/tools/config/interface/service.config -m 0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8 -s 0xfc08e5bfebdcf7bb4cf5aafc29be03c1d53898f1 -f "transfer(address,uint256)" -a 0x1be8e78d765a2e63339fc99a66320db73158a35a,100
