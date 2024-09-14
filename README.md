# Smart Contracts GraphQL API

This repository provides a GraphQL API that allows users to make API calls to a running `smart-contracts-cli` backend. The API supports various functions to interact with smart contracts, including creating accounts, compiling contracts, deploying contracts, and executing contract functions.

## Prerequisites

Before you begin, ensure you have the following prerequisites:

1. **ResilientDB**: A running instance of ResilientDB with the smart contracts service running. You can find more information and setup instructions here: [ResilientDB](https://github.com/apache/incubator-resilientdb)

2. **Smart Contracts CLI**: A running instance of the smart-contracts-cli. You can find more information and setup instructions here: [smart-contracts-cli](https://github.com/ResilientEcosystem/ResContract).

## Installation

To set up the project, follow these steps:

1. **Clone the repository**:
    ```sh
    git clone https://github.com/yourusername/smart-contracts-graphql.git
    cd smart-contracts-graphql
    ```

2. **Install dependencies**:
    ```sh
    npm install
    ```

3. **Start the server**:
    ```sh
    node server.js
    ```

The server will start running on port 4000. You can access the GraphQL API at `http://localhost:4000/graphql`.

## GraphQL API

The GraphQL API supports the following queries:

### `createAccount`

Creates a new account using the specified configuration file.

**Arguments**:
- `config` (String): Path to the configuration file.


### `compileContract`

Compiles a smart contract from the specified source path and outputs the compiled contract to the specified output path.

**Arguments**:

-   `sourcePath`  (String): Path to the source file of the smart contract.
-   `outputPath`  (String): Path to the output file for the compiled contract.

### `deployContract`

Deploys a smart contract using the specified configuration, contract file, contract name, arguments, and owner address.

**Arguments**:

-   `config`  (String): Path to the configuration file.
-   `contract`  (String): Path to the contract file.
-   `name`  (String): Name of the contract.
-   `arguments`  (String): Arguments for the contract constructor.
-   `owner`  (String): Owner address.



### `executeContract`

Executes a function of a deployed smart contract.

**Arguments**:

-   `config`  (String): Path to the configuration file.
-   `sender`  (String): Address of the sender.
-   `contract`  (String): Name of the contract.
-   `function`  (String): Name of the function to execute.
-   `arguments`  (String): Arguments for the function.

## Sample Queries

Here are some sample queries you can run:

1.  **Create Account**:
    
    ```graphql
    {
      createAccount(config: "incubator-resilientdb/service/tools/config/interface/service.config")
    }
    ```
	**Explanation**: This query creates a new account using the specified configuration file. The `config` argument points to the path of the configuration file that contains necessary settings for creating the account. The function returns a string indicating the success or failure of the account creation process.
    
2.  **Compile Contract**:
    
    ```graphql
    {
      compileContract(sourcePath: "contracts/MyContract.sol", outputPath: "build/MyContract.json")
    }
    ```
	**Explanation**: This query compiles a smart contract from the specified source file and outputs the compiled contract to the specified output file. The `sourcePath` argument is the path to the source file of the smart contract (in this case, `contracts/MyContract.sol`), and the `outputPath` argument is the path where the compiled contract will be saved (in this case, `build/MyContract.json`). The function returns a string indicating the success or failure of the compilation process.
    
3.  **Deploy Contract**:
    
    ```graphql
    {
      deployContract(
        config: "incubator-resilientdb/service/tools/config/interface/service.config",
        contract: "build/MyContract.json",
        name: "MyContract",
        arguments: "1000",
        owner: "0x1be8e78d765a2e63339fc99a66320db73158a35a"
      )
    }
    ```
    **Explanation**: This query deploys a smart contract using the specified configuration, contract file, contract name, arguments, and owner address. The function returns a string indicating the success or failure of the deployment process.
    
4.  **Execute Contract**:
    
    ```graphql
    {
      executeContract(
        config: "incubator-resilientdb/service/tools/config/interface/service.config",
        sender: "0x1be8e78d765a2e63339fc99a66320db73158a35a",
        contract: "MyContract",
        function: "transfer",
        arguments: "0xRecipientAddress,100"
      )
    }
    ```
    **Explanation**: This query executes a function of a deployed smart contract. The arguments are as follows:

	-   `config`: Path to the configuration file.
	-   `sender`: Address of the sender who is executing the contract function.
	-   `contract`: Name of the contract.
	-   `function`: Name of the function to execute (in this case,  `transfer`).
	-   `arguments`: Arguments for the function (in this case, transferring  `100`  tokens to  `0xRecipientAddress`).
