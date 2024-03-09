import os
import sys
current_file_path = os.path.abspath(__file__)
current_dir = os.path.dirname(current_file_path)
new_path_dir = os.path.join(current_dir, "bazel-out", "k8-fastbuild", "bin")
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
