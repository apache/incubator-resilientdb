import sys, os, time, subprocess

subprocess.run("bazel build :pybind_kv_so", shell=True)
sys.path.append(os.getcwd())

from kv_operation import set_value, get_value, get_value_readonly

print(set_value("test", "111225"))
time.sleep(0.5)
print(f"Key 'test' value: {get_value_readonly('test')}")