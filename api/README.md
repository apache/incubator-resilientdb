<!--
  - Licensed to the Apache Software Foundation (ASF) under one
  - or more contributor license agreements.  See the NOTICE file
  - distributed with this work for additional information
  - regarding copyright ownership.  The ASF licenses this file
  - to you under the Apache License, Version 2.0 (the
  - "License"); you may not use this file except in compliance
  - with the License.  You may obtain a copy of the License at
  -
  -   http://www.apache.org/licenses/LICENSE-2.0
  -
  - Unless required by applicable law or agreed to in writing,
  - software distributed under the License is distributed on an
  - "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  - KIND, either express or implied.  See the License for the
  - specific language governing permissions and limitations
  - under the License.
  -->

# ResilientDB kv-Service Python API(Get and Set Command)

## Description
This API allows users to use kv-service of the ResilientDB in Python directly.

## How to Run
1. Make sure to run `./INSTALL.sh` in advance.
1. cd to `incubator-resilientdb/api` folder.
2. Run command `bazel build :pybind_kv_so`.
3. From `kv_operation.py`, import `get_value` and `set_value` functions into your Python file to use it (Make sure to use the same Python version when running `bazel build` command and calling the functions).

## Parameters
### `set_value`:
1. `key`: The key user wants to store in a key-value pair. Acceptable types are `str`, `int`, `float`.
2. `value`: The corresponding value to `key` in the key-value pair. Acceptable types are `str`, `int`, `float`.
3. config_path (optional): The path to the user's blockchain config file (IP addresses). If the user does not specify this parameter, the system will default to the address located in "ip.address.config." The acceptable type is `str`.
4. `return`: `True` if `value` has been set successfully; otherwise, `value` has not been set successfully.
### `get_value`:
1. `key`: The key user wants to get in a key-value pair. Acceptable types are `str`, `int`, `float`.
2. `return`: `\n` if the corresponding value of `key` is empty, otherwise is the corresponding value of `key`.


## Example
```angular2html
import sys
# Your path to ResilientDB api folder
sys.path.append("/home/ubuntu/Desktop/incubator-resilientdb/api")
from kv_operation import set_value, get_value

set_value("test", "111222")
get_value("test")
```

If having set up the environment parameter, "sys.path" can be ignorred.
```
export PYTHON_PATH="/home/ubuntu/Desktop/incubator-resilientdb/api":PYTHON_PATH
```
```angular2html
from kv_operation import set_value, get_value

set_value("test", "111222")
get_value("test")
```




