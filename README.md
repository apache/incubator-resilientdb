# Smart Contracts GraphQL API 

This repository provides a GraphQL API that allows users to interact with smart contracts in the ResilientDB ecosystem through a set of GraphQL queries and mutations. The API leverages the `rescontract-cli` tool to perform various functions, including creating accounts, compiling contracts, deploying contracts, and executing contract functions.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Configuration](#configuration)
- [Running the Server](#running-the-server)
- [GraphQL API](#graphql-api)
  - [Mutations](#mutations)
- [Sample Mutations](#sample-mutations)
	- [Using Type “path”](#using-type-path)
	- [Using Type “data”](#using-type-data)
- [Contributing](#contributing)
- [License](#license)

## Prerequisites

Before you begin, ensure you have the following prerequisites:

1. **ResilientDB**: A running instance of ResilientDB with the smart contracts service running. More information and setup instructions can be found here: [ResilientDB](https://github.com/apache/incubator-resilientdb)

2. **ResContract CLI**: Install the `rescontract-cli` tool globally. Follow the instructions in the [ResContract CLI Repository](https://github.com/ResilientEcosystem/ResContract) to install and configure it.

   ```bash
   npm install -g rescontract-cli
   ```

3.  **Node.js (version >= 15.6.0)**:  Download and install Node.js version 15.6.0 or higher, as the application uses crypto.randomUUID() which was introduced in Node.js v15.6.0.

	```bash
	# You can check your Node.js version with:
	node -v
	```


**The prerequisites listed above can be installed using the `INSTALL.sh` script:**

   ```bash
   chmod +x INSTALL.sh
   ./INSTALL.sh
   ```

## Installation

To set up the project, follow these steps:

1.  **Clone the repository**:
    
	```bash
    git clone https://github.com/ResilientEcosystem/smart-contracts-graphql.git
    cd smart-contracts-graphql
    ```
    
2.  **Install dependencies**:
    
    ```bash
	 npm install
	```

## Configuration

Ensure that the  `ResContract CLI`  is properly configured. You need to set the  `ResDB_Home`  environment variable or provide a  `config.yaml`  file as required by the CLI.

Refer to the  [ResContract CLI README](https://github.com/ResilientEcosystem/ResContract#configuration-)  for detailed instructions.

## Running the Server

Start the server using the following command:

```bash
npm start
```

The server will start running on port  `8400`. You can access the GraphQL API at  `http://localhost:8400/graphql`.

## GraphQL API

The GraphQL API supports the following mutations to interact with smart contracts.

### Mutations

#### `createAccount`

Creates a new account using the specified configuration file.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Configuration data or path to the configuration file.
-   `type`  (String): Optional. Specifies whether config is data or a file path. Accepts "data" or "path". Defaults to "path".

**Returns**: String - Address of the newly created account or an error message.

#### `compileContract`

Compiles a smart contract from the specified source.

**Type**: Mutation

**Arguments**:

-   `source`  (String!): Solidity code or path to the source file of the smart contract.
-   `type`  (String): Optional. Specifies whether config is data or a file path. Accepts "data" or "path". Defaults to "path".

**Returns**: String - Name of the compiled contract JSON file or an error message.

#### `deployContract`

Deploys a smart contract using the specified configuration, contract file, contract name, arguments, and owner address.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Configuration data or path to the configuration file.
-   `contract`  (String!): Path to the contract JSON file.
-   `name`  (String!): Name of the contract (include the full path and contract name).
-   `arguments`  (String!): Constructor arguments for the contract (comma-separated).
-   `owner`  (String!): Owner address.
-   `type`  (String): Optional. Specifies whether config is data or a file path. Accepts "data" or "path". Defaults to "path".

**Returns**: String - Details of the deployed contract or an error message.

#### `executeContract`

Executes a function of a deployed smart contract.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Configuration data or path to the configuration file.
-   `sender`  (String!): Address of the sender.
-   `contract`  (String!): Address of the deployed contract.
-   `functionName`  (String!): Name of the function to execute (include parameter types).
-   `arguments`  (String!): Arguments for the function (comma-separated).
-   `type`  (String): Optional. Specifies whether config is data or a file path. Accepts "data" or "path". Defaults to "path".

**Returns**: String - Result of the function execution or an error message.

## Sample Mutations

Below are sample mutations demonstrating how to use the API with both "path" and "data" types.

### Using type `path` 

The `type` option defaults to "path" and doesn't have to be explicitly set. 

1. **Create Account**

```graphql
mutation {
  createAccount(config: "../incubator-resilientdb/service/tools/config/interface/service.config")
}
```

**Sample Response:**
```json
{
  "data": {
    "createAccount": "0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8"
  }
}
```

**Explanation**: Creates a new account using the configuration file located at the specified path.

2.  **Compile Contract**
```graphql
mutation {
  compileContract(
    source: "token.sol"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "compileContract": "Compiled successfully to /users/yourusername/smart-contracts-graphql/compiled_contracts/MyContract.json"
  }
}
```
**Explanation**: Compiles the smart contract located at token.sol and saves the compiled JSON to the compiled_contracts directory.

3.  **Deploy Contract**
```graphql
mutation {
  deployContract(
    config: "../incubator-resilientdb/service/tools/config/interface/service.config",
    contract: "compiled_contracts/MyContract.json",
    name: "token.sol:Token",
    arguments: "1000",
    owner: "0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8"
  )
  {
    ownerAddress
    contractAddress
    contractName
  }
}
```

**Sample Response:**
```json
{
  "data": {
    "deployContract": {
      "ownerAddress": "0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8",
      "contractAddress": "0xfc08e5bfebdcf7bb4cf5aafc29be03c1d53898f1",
      "contractName": "token.sol:Token"
    }
  }
}
```
**Explanation**: Deploys the compiled contract using the specified parameters.

4.  **Execute Contract**
```graphql
mutation {
  executeContract(
    config: "../incubator-resilientdb/service/tools/config/interface/service.config",
    sender: "0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8",
    contract: "0xfc08e5bfebdcf7bb4cf5aafc29be03c1d53898f1",
    functionName: "transfer(address,uint256)",
    arguments: "0x1be8e78d765a2e63339fc99a66320db73158a35a,100"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "executeContract": "Execution successful"
  }
}
```
**Explanation**: Executes the transfer function of the deployed contract, transferring 100 tokens to the specified address.


### Using type `data` 

In this mode, the API accepts configuration data and Solidity code directly, without needing file paths.

1. **Create Account**

```graphql
mutation {
  createAccount(
    config: "5 127.0.0.1 10005",
    type: "data"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "createAccount": "0x255d051758e95ed4abb2cdc69bb454110e827441"
  }
}
```

**Explanation**: Creates a new account using the provided configuration data.

2.  **Compile Contract**
```graphql
mutation {
  compileContract(
    source: """
    pragma solidity ^0.8.0;
		...
		...
    }
    """,
    type: "data"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "compileContract": "contract-20f42b42-f56f-45e8-8264-33cf8d93f8be.json"
  }
}
```
**Explanation**: Compiles the provided Solidity code and returns the name of the compiled contract JSON file stored in the compiled_contracts directory.

3.  **Deploy Contract**
```graphql
mutation {
  deployContract(
    config: "5 127.0.0.1 10005",
    contract: "contract-20f42b42-f56f-45e8-8264-33cf8d93f8be.json",
    name: "Token",
    arguments: "1000",
    owner: "0x255d051758e95ed4abb2cdc69bb454110e827441",
    type: "data"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "deployContract": "owner_address: 0x255d051758e95ed4abb2cdc69bb454110e827441\ncontract_address: 0xb616e4c564b03fe336333758739a7d7ee0227d5d\ncontract_name: Token"
  }
}
```
**Explanation**: Deploys the compiled contract using the configuration data and the compiled contract JSON file name returned from the compileContract mutation.

4.  **Execute Contract**
```graphql
mutation {
  executeContract(
    config: "5 127.0.0.1 10005",
    sender: "0x255d051758e95ed4abb2cdc69bb454110e827441",
    contract: "0xb616e4c564b03fe336333758739a7d7ee0227d5d",
    functionName: "transfer(address,uint256)",
    arguments: "0x213ddc8770e93ea141e1fc673e017e97eadc6b96,100",
    type: "data"
  )
}
```

**Sample Response:**
```json
{
  "data": {
    "executeContract": "result: 0x0000000000000000000000000000000000000000000000000000000000000001"
  }
}
```
**Explanation**: Executes the transfer function of the deployed contract, transferring 100 tokens to the specified address, using configuration data.

## License

This project is licensed under the Apache License - see the  [LICENSE](LICENSE)  file for details.

