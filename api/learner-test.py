import sys, os, time, subprocess

subprocess.run("bazel build :pybind_kv_so", shell=True)
sys.path.append(os.getcwd())

from kv_operation import set_value, get_value, get_value_readonly

for i in range(0,10):
    print(set_value("INC_TEST", "0"))
    time.sleep(0.2)

for i in range(0,10):
    print(set_value("INC_TEST", str(i+1)))
    time.sleep(0.2)
    print(f"Key 'INC_TEST' value: {get_value_readonly('INC_TEST')}")
    time.sleep(0.2)

