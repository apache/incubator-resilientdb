import sys, os, time
# Your path to ResilientDB api folder
sys.path.append(os.getcwd())
from kv_operation import set_value, get_value

print(set_value("test", "111225"))
time.sleep(0.5)
print(f"Key 'test' value: {get_value('test')}")