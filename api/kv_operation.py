#
 # Licensed to the Apache Software Foundation (ASF) under one
 # or more contributor license agreements.  See the NOTICE file
 # distributed with this work for additional information
 # regarding copyright ownership.  The ASF licenses this file
 # to you under the Apache License, Version 2.0 (the
 # "License"); you may not use this file except in compliance
 # with the License.  You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing,
 # software distributed under the License is distributed on an
 # "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 # KIND, either express or implied.  See the License for the
 # specific language governing permissions and limitations
 # under the License.
 #

import os
import sys
current_file_path = os.path.abspath(__file__)
current_dir = os.path.dirname(current_file_path)
parent_dir = os.path.dirname(current_dir)
new_path_dir = os.path.join(parent_dir, "bazel-out", "k8-fastbuild", "bin", "api")
sys.path.insert(0, new_path_dir)
import pybind_kv


def set_value(key: str or int or float, value: str or int or float, config_path: str = current_dir + "/ip_address.config") -> bool:
    """
    :param key: The key you want to set your value to.
    :param value: The key's corresponding value in key value pair.
    :param config_path: Default is connect to the main chain, users can specify the path to connect to their local blockchain.
    :return: True if value has been set successfully.
    """
    return pybind_kv.set(str(key), str(value), os.path.abspath(config_path))


def get_value(key: str or int or float, config_path: str = current_dir + "/ip_address.config") -> str:
    """
    :param key: The key of the value you want to get in key value pair.
    :param config_path: Default is connect to the main chain, users can specify the path to connect to their local blockchain.
    :return: A string of the key's corresponding value.
    """
    return pybind_kv.get(str(key), os.path.abspath(config_path))
