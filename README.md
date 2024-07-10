# ResDB Smart Contracts CLI 🚀

The ResDB Smart Contracts CLI is a tool for creating, deploying, and managing smart contracts within the ResilientDB ecosystem. It is designed to work seamlessly with other ResilientDB projects, providing a streamlined interface for developers.

## Features ✨

- **Create Smart Contracts**: Generate new smart contract templates.
- **Deploy Smart Contracts**: Deploy contracts to the blockchain.
- **Manage Contracts**: Interact with and manage deployed contracts.

## Exploring Leading Technologies 🔍

### Key Technologies

- **Truffle Framework**: A development framework for Ethereum, offering tools for crafting, testing, and deploying smart contracts with Solidity.
- **Ganache**: A personal Ethereum development blockchain, enabling contract deployment, application creation, and testing locally.
- **Metamask**: A browser extension and Ethereum wallet for interacting with Ethereum Dapps directly from the browser.

## Getting Started with ResDB Smart Contracts CLI 🚀

### Prerequisites

Before installing and using the Smart Contracts CLI, ensure you have the following prerequisites installed on your system:

#### Node.js and npm

Node.js and npm are required to run the Smart Contracts CLI. Follow the instructions below to install them based on your operating system.

#### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y nodejs npm
```
#### MacOS

```bash
brew update
brew install node
```
## Configuration Management with `config.js` ⚙️

### The Role of `ResDB_Home`

The `ResDB_Home` path points to the directory where the ResilientDB installation resides. This path allows the CLI to locate and execute ResilientDB-related binaries and scripts.

### `config.js` Implementation

The `config.js` file contains logic to prompt the user for the `ResDB_Home` path the first time they use the CLI and then stores this path for future use. Here’s how it works:

1. **Environment Variable Check**: Checks if the `ResDB_Home` environment variable is already set.
2. **Configuration File Check**: Checks a configuration file (`~/.smart-contracts-cli-config.json`) to see if the `ResDB_Home` path has been saved previously.
3. **User Prompt**: Prompts the user to enter the `ResDB_Home` path if neither the environment variable nor the configuration file provides it.
4. **Saving the Path**: Stores the provided path in both the environment variable and the configuration file.

<details>
<summary>config.js code</summary>

```javascript
const path = require('path');
const inquirer = require('inquirer');
const fs = require('fs-extra');
const os = require('os');

const CONFIG_FILE_PATH = path.join(os.homedir(), '.smart-contracts-cli-config.json');

async function getResDBHome() {
  if (process.env.ResDB_Home) {
    return process.env.ResDB_Home;
  }

  if (await fs.pathExists(CONFIG_FILE_PATH)) {
    const config = await fs.readJson(CONFIG_FILE_PATH);
    if (config.resDBHome) {
      process.env.ResDB_Home = config.resDBHome;
      return config.resDBHome;
    }
  }

  return null;
}

async function setResDBHome(resDBHome) {
  process.env.ResDB_Home = resDBHome;
  await fs.writeJson(CONFIG_FILE_PATH, { resDBHome });
}

async function promptForResDBHome() {
  const answers = await inquirer.prompt([
    {
      type: 'input',
      name: 'resDBHome',
      message: 'Please enter the ResDB_Home path:',
    },
  ]);

  const resDBHome = answers.resDBHome;
  await setResDBHome(resDBHome);

  return resDBHome;
}

module.exports = {
  getResDBHome,
  setResDBHome,
  promptForResDBHome,
};
```

</details>

## Usage
To get started with the ResDB Smart Contracts CLI:

1. Clone the repository.
2. Install the dependencies with `npm install`.
3. Run the CLI with `smart-contracts-cli <command> <options>`.
4. Follow the prompts to set up your `ResDB_Home` path.

## Commands

### Create Command

The `create` command initializes a new account using ResilientDB's smart contract tools.

#### Usage

```bash
smart-contracts-cli create --config <path_to_config>
```

- `path_to_config`: Path to the configuration file.

### Compile Command

The compile command compiles a Solidity smart contract into a JSON file using solc.

#### Usage

```bash
smart-contracts-cli compile <inputFile.sol> <outputFile.json>
```

- `inputFile.sol`: Path to the Solidity smart contract file.
- `outputFile.json`: Name of the resulting JSON file.

Make sure solc (Solidity compiler) is installed on your system. Refer to the [Prerequisites](#prerequisites) section in the README for installation instructions.

### Deploy Command

The `deploy` command deploys the smart contract. 

#### Usage

```bash
smart-contracts-cli deploy --config <service.config> --contract <contract.json> \
--name <tokenName> --arguments <parameters> --owner <address> 
```

- `service.config`: Client configuration path
- `contract.json`: Path to the contract json file
- `tokenName`: Name of the contract created
- `parameters`: Parameters to create the contract object
- `address`: Contract owner's address

### Execute Command

The `execute` command executes a smart contract function using ResilientDB's smart contract tools.

#### Usage

```bash
smart-contracts-cli execute --config <service.config> --sender <senderAddress> \
--contract <contractAddress> --function <functionName> --arguments <parameters>
```

- `service.config`: Path to the client configuration file.
- `senderAddress`: Address of the sender executing the function.
- `contractAddress`: Address of the deployed contract.
- `functionName`: Name of the function to execute.
- `parameters`: Arguments to pass to the function.

Example:
```bash 
smart-contracts-cli execute --config service/tools/config/interface/service.config \
--sender 0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8 \
--contract 0xfc08e5bfebdcf7bb4cf5aafc29be03c1d53898f1 \
--function "transfer(address,uint256)" \
--arguments "0x1be8e78d765a2e63339fc99a66320db73158a35a,100"

```
