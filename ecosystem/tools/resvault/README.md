#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

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
    <a href="https://github.com/ResilientApp/ResVault/commits/main"><img alt="GitHub commit activity (branch)" src="https://img.shields.io/github/commit-activity/w/ResilientApp/ResVault">
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

ResVault is a chrome extension that serves as a wallet for [ResilientDB](https://resilientdb.com). ResilientDB is a High Throughput Yielding Permissioned Blockchain Fabric founded by [ExpoLab](https://expolab.org/) at [UC Davis](https://www.ucdavis.edu/) in 2018. ResilientDB advocates a system-centric design by adopting a multi-threaded architecture that encompasses deep pipelines. Further, ResilientDB separates the ordering of client transactions from their execution, which allows it to process messages out-of-order.

- Create Account
- Delete Account
- Login
- Submit Transactions
- Transactions logging
- User Profiles
- Multi-account support

**Pending**:
- [ ] Password improvement
- [ ] Transaction details
- [ ] View all transactions

## Installation
### Via GitHub release
Open [chrome://extensions/](chrome://extensions/) in Google Chrome, toggle Developer mode on:
- Click on Load unpacked
- Select the build folder that you downloaded from the GitHub releases.

### Via Chrome Webstore
Coming Soon

## Build
**NodeJS is required.**  
Open `terminal` and execute:
```shell
git clone https://github.com/ResilientApp/ResVault.git
cd ResVault
npm install
npm run build
```

## Example Usage
#### Demo Video
Coming Soon

## Links

- [Website](https://resilientdb.com)

## Contributing

Before creating an issue, please ensure that it hasn't already been reported/suggested.

The issue tracker is only for bug reports and enhancement suggestions. If you have a question, please reach out to [apratim@expolab.org](apratim@expolab.org) instead of opening an issue – you will get redirected there anyway.

If you wish to contribute to the ResVault codebase or documentation, feel free to fork the repository and submit a pull request.

## Help 

If you don't understand something in the documentation, you are experiencing problems, or you just need a gentle
nudge in the right direction, please don't hesitate to reach out to [apratim@expolab.org](apratim@expolab.org).
