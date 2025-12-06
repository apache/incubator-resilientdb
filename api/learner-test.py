import sys, os, time, subprocess

subprocess.run("bazel build :pybind_kv_so", shell=True)
sys.path.append(os.getcwd())

from kv_operation import set_value, get_value, get_value_readonly

for i in range(0,10):
    print(set_value("test", "342"))
    time.sleep(0.2)

print(f"Key 'test' value: {get_value_readonly('test')}")