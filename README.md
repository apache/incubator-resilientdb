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
- [Contributing](#contributing)
- [License](#license)

## Prerequisites

Before you begin, ensure you have the following prerequisites:

1. **ResilientDB**: A running instance of ResilientDB with the smart contracts service running. More information and setup instructions can be found here: [ResilientDB](https://github.com/apache/incubator-resilientdb)

2. **ResContract CLI**: Install the `rescontract-cli` tool globally. Follow the instructions in the [ResContract CLI Repository](https://github.com/ResilientEcosystem/ResContract) to install and configure it.

   ```bash
   npm install -g rescontract-cli
   ```

3.  **Node.js (version >= 14)**:  Download and install Node.js

## Installation

To set up the project, follow these steps:

1.  **Clone the repository**:
    
	```bash
    git clone https://github.com/yourusername/smart-contracts-graphql.git
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

The server will start running on port  `4000`. You can access the GraphQL API at  `http://localhost:4000/graphql`.

## GraphQL API

The GraphQL API supports the following mutations to interact with smart contracts.

### Mutations

#### `createAccount`

Creates a new account using the specified configuration file.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Path to the configuration file.

**Returns**: String - Address of the newly created account or an error message.

#### `compileContract`

Compiles a smart contract from the specified source path and outputs the compiled contract to the specified output path.

**Type**: Mutation

**Arguments**:

-   `sourcePath`  (String!): Path to the source file of the smart contract.
-   `outputPath`  (String!): Path to the output file for the compiled contract.

**Returns**: String - Success message or an error message.

#### `deployContract`

Deploys a smart contract using the specified configuration, contract file, contract name, arguments, and owner address.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Path to the configuration file.
-   `contract`  (String!): Path to the contract JSON file.
-   `name`  (String!): Name of the contract (include the full path and contract name).
-   `arguments`  (String!): Constructor arguments for the contract (comma-separated).
-   `owner`  (String!): Owner address.

**Returns**: String - Details of the deployed contract or an error message.

#### `executeContract`

Executes a function of a deployed smart contract.

**Type**: Mutation

**Arguments**:

-   `config`  (String!): Path to the configuration file.
-   `sender`  (String!): Address of the sender.
-   `contract`  (String!): Address of the deployed contract.
-   `functionName`  (String!): Name of the function to execute (include parameter types).
-   `arguments`  (String!): Arguments for the function (comma-separated).

**Returns**: String - Result of the function execution or an error message.

## Sample Mutations

Here are some sample mutations you can run:

### Create Account

```graphql
mutation {
  createAccount(config: "../path/to/service.config")
}
``` 

**Sample Response**:

```json
{
  "data": {
    "createAccount": "0x3b706424119e09dcaad4acf21b10af3b33cde350"
  }
}
```
**Explanation**: This mutation creates a new account using the specified configuration file. The response contains the address of the newly created account.

### Compile Contract

```graphql
mutation {
  compileContract(
    sourcePath: "/path/to/token.sol",
    outputPath: "output.json"
  )
}
```
**Sample Response**:

```json
{
  "data": {
    "compileContract": "Compiled successfully to output.json"
  }
}
``` 

**Explanation**: This mutation compiles the smart contract located at  `/path/to/token.sol`and outputs the compiled contract to  `output.json`.

### Deploy Contract

```graphql
mutation {
  deployContract(
    config: "../path/to/service.config",
    contract: "output.json",
    name: "token.sol:Token",
    arguments: "1000",
    owner: "0x3b706424119e09dcaad4acf21b10af3b33cde350"
  )
}
``` 

**Sample Response**:

```json
{
  "data": {
    "deployContract": "owner_address: \"0x3b706424119e09dcaad4acf21b10af3b33cde350\"\ncontract_address: \"0xc975ab41e0c2042a0229925a2f4f544747fd66cd\"\ncontract_name: \"token.sol:Token\""
  }
}
``` 

**Explanation**: This mutation deploys the compiled contract using the specified parameters. The response includes the owner address, contract address, and contract name.

### Execute Contract

```graphql
mutation {
  executeContract(
    config: "../path/to/service.config",
    sender: "0x3b706424119e09dcaad4acf21b10af3b33cde350",
    contract: "0xc975ab41e0c2042a0229925a2f4f544747fd66cd",
    functionName: "transfer(address,uint256)",
    arguments: "0x4847155cbb6f2219ba9b7df50be11a1c7f23f829,100"
  )
} 
```

**Sample Response**:

```json
{
  "data": {
    "executeContract": "Function executed successfully."
  }
}
``` 

**Explanation**: This mutation executes the  `transfer`  function of the deployed contract, transferring  `100`  tokens to  `0x4847155cbb6f2219ba9b7df50be11a1c7f23f829`.

## License

This project is licensed under the Apache License - see the  [LICENSE](LICENSE)  file for details.
