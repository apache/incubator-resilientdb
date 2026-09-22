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
# bazel-bin points at the build output for whatever platform was built.
sys.path.insert(0, os.path.join(parent_dir, "bazel-bin", "api"))
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


def create_index_entry(index_name: str, attributes, primary_key: str,
                       config_path: str = current_dir + "/ip_address.config") -> int:
    """
    Add a secondary index entry, e.g. create_index_entry("by_city", "Davis", "user:1").

    :param index_name: The index the entry belongs to, chosen by you.
    :param attributes: One attribute value, or a list of them for a multi-attribute index.
    :param primary_key: The key of the record being indexed. It must already exist.
    :param config_path: Path to the client config.
    :return: 0 on success, -3 if the servers rejected it, -1 or -2 if it could not be sent.
    """
    return pybind_kv.create_index_entry(index_name, attributes, str(primary_key),
                                        os.path.abspath(config_path))


def delete_index_entry(index_name: str, attributes, primary_key: str,
                       config_path: str = current_dir + "/ip_address.config") -> int:
    """
    Remove a secondary index entry. The record itself is not deleted.

    :return: 0 on success, -3 if no such entry exists, -1 or -2 if it could not be sent.
    """
    return pybind_kv.delete_index_entry(index_name, attributes, str(primary_key),
                                        os.path.abspath(config_path))


def update_index_entry(index_name: str, old_attributes, new_attributes, primary_key: str,
                       config_path: str = current_dir + "/ip_address.config") -> int:
    """
    Move an entry to new attributes, e.g. when a record's city changes. The old entry is
    removed and the new one added in a single write.

    :return: 0 on success, -3 if the old entry does not exist, -1 or -2 if it could not be sent.
    """
    return pybind_kv.update_index_entry(index_name, old_attributes, new_attributes,
                                        str(primary_key), os.path.abspath(config_path))


def query_by_index(index_name: str, attributes,
                   config_path: str = current_dir + "/ip_address.config") -> list:
    """
    Primary keys whose entry starts with the given attributes, each once and sorted.

    Attributes match from the left: in an index built as (owner, format), you can query
    by owner, or by owner and format, but not by format alone. An empty list returns
    every entry in the index.

    :return: A list of primary keys, empty if nothing matched or the request failed.
    """
    return pybind_kv.query_by_index(index_name, attributes, os.path.abspath(config_path))
