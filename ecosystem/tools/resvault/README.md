<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-->
---
<div align="center">
  <br />
  <p>
    <a href="https://resilientdb.com"><img src="https://i.imgur.com/s4089K7.png" width="300" alt="resvault" /></a>
  </p>
  <br />
  <p>
    <a href="https://github.com/ResilientApp/ResVault/releases/"><img alt="GitHub Release Date - Published_At" src="https://img.shields.io/github/release-date/ResilientApp/ResVault">
    </a>
    <a href="https://github.com/ResilientApp/ResVault/actions"><img alt="GitHub Workflow Status (with event)" src="https://img.shields.io/github/actions/workflow/status/ResilientApp/ResVault/release.yml">
    </a>
    <a href="https://github.com/ResilientApp/ResVault/commits/main"><img alt="GitHub commit activity (branch)" src="https://img.shields.io/github/commit-activity/ResilientApp/ResVault">
    </a>
    <a href="https://github.com/ResilientApp/ResVault/blob/main/LICENSE"><img alt="GitHub" src="https://img.shields.io/github/license/ResilientApp/ResVault">
    </a>
    <a href="https://github.com/ResilientApp/ResVault/releases/"><img alt="GitHub all releases" src="https://img.shields.io/github/downloads/ResilientApp/ResVault/total">
    </a>
    <a href="https://github.com/ResilientApp/ResVault/issues"><img alt="GitHub issues" src="https://img.shields.io/github/issues/ResilientApp/ResVault">
    </a>
  </p>
</div>

## About

ResVault is a Chrome extension that serves as a comprehensive wallet for [ResilientDB](https://resilientdb.com). ResilientDB is a High Throughput Yielding Permissioned Blockchain Fabric founded by [ExpoLab](https://expolab.org/) at [UC Davis](https://www.ucdavis.edu/) in 2018. ResilientDB advocates a system-centric design by adopting a multi-threaded architecture that encompasses deep pipelines. Further, ResilientDB separates the ordering of client transactions from their execution, which allows it to process messages out-of-order.

## Features

### Core Wallet Functionality
- **Create Account** - Generate new wallet accounts with secure key management
- **Delete Account** - Remove accounts with proper cleanup
- **Login/Logout** - Secure authentication system
- **Submit Transactions** - Send transactions to ResilientDB network
- **Transaction Logging** - Complete transaction history and audit trail
- **User Profiles** - Manage multiple user identities
- **Multi-account Support** - Handle multiple wallet accounts simultaneously

### Smart Contract Integration (v1.2.0)
- **Contract Deployment** - Deploy Solidity smart contracts directly from the wallet
- **Address Ownership** - Contracts are deployed using your wallet address for proper ownership
- **GraphQL Integration** - Seamless communication with ResilientDB smart contract service
- **Solidity Compilation** - Automatic compilation of Solidity contracts before deployment
- **Enhanced Error Handling** - Improved debugging and error reporting for contract operations
- **Network Flexibility** - Connect to mainnet or your local ResilientDB server

### Security & Performance
- **Secure Key Management** - Ed25519 key pairs with proper encryption
- **Transaction Validation** - Built-in validation for all operations
- **Network Connectivity** - Support for custom ResilientDB network endpoints
- **Real-time Updates** - Live transaction status and balance updates

**Pending Features**:
- [ ] Password improvement
- [ ] Transaction details view
- [ ] View all transactions
- [ ] Contract execution and interaction
- [ ] Contract interaction history
- [ ] Gas estimation and optimization

## Installation
### Via GitHub release
Open [chrome://extensions/](chrome://extensions/) in Google Chrome, toggle Developer mode on:
- Click on Load unpacked
- Select the build folder that you downloaded from the GitHub releases.

### Via Chrome Web Store
**🎉 Now Available on Chrome Web Store!**

[![Chrome Web Store](https://img.shields.io/badge/Chrome%20Web%20Store-Available-green)](https://chromewebstore.google.com/detail/resvault/ejlihnefafcgfajaomeeogdhdhhajamf)

**[Install ResVault from Chrome Web Store](https://chromewebstore.google.com/detail/resvault/ejlihnefafcgfajaomeeogdhdhhajamf)**


## Build
**NodeJS is required.**  
Open `terminal` and execute:
```shell
git clone https://github.com/ResilientApp/ResVault.git
cd ResVault
npm install --legacy-peer-deps
CI= npm run build
```

### Build and test in Chrome (this repo)
To build ResVault from this repo and load it in Chrome for testing (before publishing to the Chrome Web Store):

1. **Install dependencies and build**
   ```shell
   cd ecosystem/tools/resvault
   npm install --legacy-peer-deps
   npm run build
   ```
   This produces a `build` folder with the extension (React app in `index.html` + `static/js/`, `manifest.json`, `content.js`, `background.js`, and icons). **You must run the full build**—loading only the `public` folder or an incomplete `build` will show a blank popup.

2. **Load the extension in Chrome**
   - Open [chrome://extensions/](chrome://extensions/) in Google Chrome.
   - Turn **Developer mode** on (toggle in the top-right).
   - Click **Load unpacked**.
   - Choose the **`build`** folder (e.g. `incubator-resilientdb/ecosystem/tools/resvault/build`).

3. **Test**
   - Use the ResVault icon in the toolbar to open the wallet popup and test flows.
   - After changes, run `npm run build` again, then in `chrome://extensions/` click the **Refresh** icon on the ResVault card to reload.

4. **Publish to Chrome Web Store**
   - When ready, zip the contents of the `build` folder (not the folder itself) and upload that zip in the [Chrome Web Store Developer Dashboard](https://chrome.google.com/webstore/devconsole).

## Smart Contract Usage

Compile/deploy uses the Smart Contract GraphQL server. That server must have **solc** installed (non-snap, e.g. the static binary from [Solidity releases](https://github.com/ethereum/solidity/releases)) and, when starting the server, set **SOLC_PATH** to that binary (e.g. `SOLC_PATH=/usr/local/bin/solc`). ResVault itself does not need any solc-related configuration.

### Deploying Contracts
1. Navigate to the **Contract** tab in ResVault
2. Enter your ResilientDB GraphQL server URL (e.g. `http://your-server:8400/graphql`)
3. Upload two files:
   - **Solidity contract (`.sol`)** – your contract source
   - **Configuration (`.json`)** – deployment config (see format below)
4. Click **Deploy** – the contract is compiled on the server and deployed using your wallet address

#### Contract file (`.sol`)
Standard Solidity source. No special naming; the config file references the contract by `filename:ContractName`.

Example (`SimpleStorage.sol`):
```solidity
pragma solidity >=0.5.0;

contract SimpleStorage {
    uint256 public value;

    constructor(uint256 _value) public {
        value = _value;
    }

    function setValue(uint256 _value) public {
        value = _value;
    }

    function getValue() public view returns (uint256) {
        return value;
    }
}
```

#### Configuration file (`.json`)
Must be valid JSON with exactly these fields:

| Field            | Description |
|------------------|-------------|
| `contract_name`  | `"<filename>:<ContractName>"` – the `.sol` filename and the contract name (e.g. `"simple.sol:SimpleStorage"`) |
| `arguments`      | Constructor arguments as a single string; comma-separated if multiple (e.g. `"100"` or `"100,0x123..."`) |

Example (`config.json`):
```json
{
  "arguments": "100",
  "contract_name": "simple.sol:SimpleStorage"
}
```

## Example Usage
#### Demo Video
Coming Soon

## Links

- [Website](https://resilientdb.com)
- [Chrome Web Store](https://chromewebstore.google.com/detail/resvault/ejlihnefafcgfajaomeeogdhdhhajamf) - Install ResVault
- [GitHub Repository](https://github.com/ResilientApp/ResVault) - Source Code

## Contributing

Before creating an issue, please ensure that it hasn't already been reported/suggested.

The issue tracker is only for bug reports and enhancement suggestions. If you have a question, please reach out to [apratim@expolab.org](apratim@expolab.org) instead of opening an issue – you will get redirected there anyway.

If you wish to contribute to the ResVault codebase or documentation, feel free to fork the repository and submit a pull request.

## Help 

If you don't understand something in the documentation, you are experiencing problems, or you just need a gentle
nudge in the right direction, please don't hesitate to reach out to [apratim@expolab.org](apratim@expolab.org).
