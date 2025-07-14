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

![](https://img.shields.io/github/v/release/ResilientApp/ResCLI)
![](https://img.shields.io/badge/language-ruby-orange.svg)
![](https://img.shields.io/badge/platform-Ubuntu20.0+-lightgrey.svg)
![GitHub](https://img.shields.io/github/license/ResilientApp/ResCLI)

# ResCLI

`ResCLI` is a command-line interface for managing ResDB instances and Python SDK instances. It provides functionalities to create, delete, view, and manage instances in a simple and efficient way.


## Table of Contents

- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Commands](#commands)

## Installation

To use `ResCLI`, you can download the binary from the [Releases](https://github.com/ResilientApp/ResCLI/releases) page on GitHub.

1. Go to the [Releases](https://github.com/ResilientApp/ResCLI/releases) page.
2. Download the latest release for your operating system (e.g., `res-cli-linux` for Linux).
3. Make the downloaded binary executable:

```bash
chmod +x res-cli
```
<!-- 
## Configuration

`res-cli` uses a configuration file (`config.ini`) to store settings such as the flaskBaseUrl. Follow the steps below to configure the CLI:

Create a configuration file named `config.ini`, Replace the flaskBaseUrl value with the appropriate Flask API server connection string.:

   ```ini
    [Server]
    flask_base_url = http://xyz:1234
    
    [User]
    current_user = bob@gmail.com
   ``` -->

## Usage

Once installed and configured, you can use res-cli to perform various actions related to ResDB and Python SDK instances. Run the CLI with the following command:

```bash
    ./res-cli
```

## Commands

- **Login**: Logs in to the specified user account.

```bash
    ./res-cli --login # Enter email and password when prompted
```

- **Logout**: Logs out from the current user account.

```bash
    ./res-cli --logout
```

- **Create Instance**: Creates a new ResDB or Python SDK instance.

```bash
    ./res-cli --create <type>
```

- **View Instances**: Displays details about running instances.

```bash
    ./res-cli --view-instances
```

- **Delete Instance**: Deletes a running ResDB or Python SDK instance.

```bash
    ./res-cli --delete <instance_id>
```

- **Current User**: Displays the currently logged-in user.

```bash
    ./res-cli --whoami
```

For more detailed information about each command, run `./res-cli --help` or `./res-cli <command> --help`.
