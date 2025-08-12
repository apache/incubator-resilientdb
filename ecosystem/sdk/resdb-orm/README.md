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


# ResDB-ORM

ResDB-ORM is a Python module designed to simplify interactions with ResilientDB's key-value store database by providing an Object-Relational Mapping (ORM) interface. This library allows developers to use basic CRUD functionalities with ease.

## Prerequisites

Before using this repository, ensure that the following services are running:

1. **kv_server**: This is the key-value store server that forms the backend for ResilientDB. Follow the instructions in the [ResilientDB repository](https://github.com/apache/incubator-resilientdb) to set it up and start it.

2. **ResilientDB-GraphQL**: This provides a GraphQL interface to interact with ResilientDB. Follow the instructions in the [ResilientDB-GraphQL repository](https://github.com/apache/incubator-resilientdb-graphql) to set it up and start it.

## Setting up Prerequisites

You can use the provided INSTALL.sh script to automatically install and set up both the kv_server and ResilientDB-GraphQL services. The script will clone the necessary repositories, build the services, and start them.

To use the INSTALL.sh script:

```bash
git clone https://github.com/ResilientEcosystem/ResDB-ORM.git
cd ResDB-ORM
./INSTALL.sh
```

## Installation

ResDB-ORM is also available on PyPI and can be installed using pip:

```bash
pip install resdb-orm
```

Once the above services are running, follow the steps below to set up and use ResDB-ORM.

### 1. Clone this Repository

```bash
git clone https://github.com/ResilientEcosystem/ResDB-ORM.git
cd ResDB-ORM
```

**Note:** The script may require sudo privileges to install dependencies and set up services.

### 2. Create and Activate a Virtual Environment

Set up a virtual environment to manage dependencies:

```bash
python3 -m venv venv
source venv/bin/activate
```

### 3. Install Dependencies

Ensure that you have all necessary dependencies installed:

```bash
pip install -r requirements.txt
```

### 4. Configure  `config.yaml`

Update the  `config.yaml`  file to point to the correct Crow endpoint. After running ResilientDB-GraphQL, you should see output similar to the following:

```scss
(2024-08-17 00:03:53) [INFO    ] Crow/1.0 server is running at <CROW_ENDPOINT> using 16 threads
(2024-08-17 00:03:53) [INFO    ] Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs. 
```
Replace  `<CROW_ENDPOINT>`  in the  `config.yaml`  file with the actual endpoint URL from the above output.

### 5. Verify Installation

Run the provided  `test.py`  script to verify that everything is set up correctly:

```bash
python test.py
```
This script will perform basic operations to ensure that the connection to the ResilientDB instance is functional.

### 6. Import the Module in Your Own Code

You can now import and use the  `ResDBORM`  module in your own projects:

```python
from resdb_orm.orm import ResDBORM
```

# Initialize the ORM
```python
orm = ResDBORM()

# Example usage: Create a new record
data = {"key": "value"}
record_id = orm.create(data)
print(f"Record created with ID: {record_id}")` 
```

## Contributing

We welcome contributions to this project! Please feel free to submit pull requests, report issues, or suggest new features.

## License

This project is licensed under the  Apache License 2.0.
