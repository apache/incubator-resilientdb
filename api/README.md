# ResilientDB kv-Service Python API(Get and Set Command)

## Description
This API allows users to directly use kv-service of the ResilientDB in Python

## How to Run
1. Make sure you have installed bazel5.0 and pybind11
2. cd to `incubator-resilientdb/api` folder
3. Run command `bazel build :pybind_kv_so`
4. From `kv_operation.py` import `get_value` and `set_value` function into your Python file to use it (Make sure to use same python version when run `bazel build` command and call the functions)

## Parameters
### `set_value`:
1. `key`: The key user want to store in key-value pair. Acceptable types are `str`, `int`, `float`
2. `value`: The `key`'s corresponding value in key-value pair. Acceptable types are `str`, `int`, `float`
3. `config_path`(optional): The absolute path to user's blockchain config file(ip addresses). If user does not specify this parameter, system will use main chain as default. Acceptable type is `str`
4. `return`: `True` if `value` has been set successfully, otherwise `value` has not been set successfully.
### `get_value`:
1. `key`: The key user want to get in key-value pair. Acceptable types are `str`, `int`, `float`
2. `return`: `\n` if the corresponding value of `key` is empty, otherwise is corresponding value of `key`


## Example
```angular2html
import sys
# Your path to ResilientDB api folder
sys.path.append("/home/ubuntu/Desktop/incubator-resilientdb/api")
from kv_operation import set_value, get_value

set_value("test", "111222")
get_value("test")
```