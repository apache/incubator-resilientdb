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

# Pybind11 information

Pybind11 allows for Python embedding within C++ code (and creating C++ bindings for Python). NexRes relies on Pybind11 to call Python code from C++.

## Requirements

This code was tested on Ubuntu 20.04.4 LTS. Python 3.10.8 was installed using brew in the location /home/ubuntu/.linuxbrew/bin/python3. Using brew helps simplify the linking process during compilation.
This location is referenced in [nexres/.bazelrc](https://github.com/msadoghi/nexres/tree/master/.bazelrc)

    build --action_env=PYTHON_BIN_PATH="/home/ubuntu/.linuxbrew/bin/python3"

Make sure you have the python libraries installed. These are used when the binary is run.

    sudo apt-get install python3.10-dev

## client.cpp

Calls example Python code. Provides an example of reading in the file print_sample.py into a string and executing the string as Python code.
Also provides an example of importing a Python module into C++ and calling one of its functions.

Make sure to execute the code from within this directory as when importing the "validator_example" module the code searches the same directory.
