# ResilientDB GraphQL
ResilientDB GraphQL Server

# Get Prepared 

(If you're using this for the first time, the steps below would be useful)

1. Install **Ubuntu 20.04** on your local machine

2. Once installed, go to File Explorer -> Linux -> Ubuntu 20.04

3. Clone this repository

4. Install python3 (version - 3.9+) and ensure pip is installed using the command

    sudo apt-get install python3-pip

5. Also make sure to install the venv module which creates a virtual Python environment that helps encapsulate the project's dependencies and prevents possible conflicts with the global Python environment. The command is:

    sudo apt-get install -y python3.10-venv

6. Create a veritual environment:

    python3 -m venv venv

7. Start a virtual environment:

    source venv/bin/activate

8. Go back from the virtual environment when you no longer need it:

    deactivate

# Using the SDK 

1. To use the SDK, you need to start a KV service first, which you can refer to the [resilientdb](https://github.com/resilientdb/resilientdb) repository and the [blog](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html). 

2. Then you should start the crow http service, which may take a few minutes at the first time.

    bazel build service/http_server:crow_service_main

    bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config

3. After starting the crow service, you can optionally test it by using the **curl** command to send HTTP requests.

    curl -X POST -d '{"id":"key1","value":"value1"}' 127.0.0.1:18000/v1/transactions/commit

    curl 127.0.0.1:18000/v1/transactions/key1

    The expected output of the two commands above are:

    `id: key1`

    `{"id":"key1","value":"value1"}`

4. Start your virtual environment

    source venv/bin/activate

5. Run the requirements.txt command to install the SDK related dependencies

    python3 -m pip install -r requirements.txt

6. Try the test script 

    python3 test_sdk.py

You have successfully run the script if you see the following output:

    The retrieved txn is successfully validated

# GraphQL

<!-- 7. (Temporary resolve) Copy app.py to the nexres_sdk folder to ensure execution -->

7. (Optional) If there is an error while running any command relating to NexRes, this command below may help:

    sudo apt-get install python3.10-distutils

Note: If there is an error with the pip version, use the command:

    curl -sS https://bootstrap.pypa.io/get-pip.py | python3.10

8. Open a venv in the Ubuntu server and run the command: 

    source venv/bin/activate

    sudo apt-get install cloud-init

9. Install the remaining dependencies in venv

    python3 -m pip install strawberry-graphql

    python3 -m pip install flask_cors

10. **BUG FIX**: in the case requirements.txt doesn't work, and the `cloud-init` version is not found, edit the file to be:
```C
aiohttp==3.8.3
aiohttp-cors==0.7.0
aiosignal==1.3.1
anyio==3.6.2
ariadne==0.17.0
async-timeout==4.0.2
attrs==22.2.0
base58==2.1.0
certifi==2022.12.7
cffi==1.15.1
charset-normalizer==2.1.1
click==8.1.3
commonmark==0.9.1
cryptoconditions==0.8.1
cryptography==3.4.7
Flask==2.2.2
Flask-Cors==3.0.10
frozenlist==1.3.3
graphql-core==3.2.3
h11==0.14.0
idna==3.4
importlib-metadata==5.2.0
itsdangerous==2.1.2
Jinja2==3.1.2
libcst==0.4.9
MarkupSafe==2.1.1
multidict==6.0.4
mypy-extensions==0.4.3
pyasn1==0.4.8
pycparser==2.21
Pygments==2.14.0
PyNaCl==1.4.0
pysha3==1.0.2
python-dateutil==2.8.2
python-multipart==0.0.5
python-rapidjson==1.8
PyYAML==6.0
requests==2.28.1
rich==13.0.1
six==1.16.0
sniffio==1.3.0
starlette==0.23.1
strawberry-graphql==0.152.0
typing-inspect==0.8.0
typing_extensions==4.4.0
urllib3==1.26.13
uvicorn==0.20.0
Werkzeug==2.2.2
yarl==1.8.2
zipp==3.11.0
```

- You may get an error that says `Running setup.py install for <dependency> did not run successfully.`
- First, try `pip3 install wheel` or `pip install wheel`
- Next, install all missing dependencies manually
    - **NOTE** as long as you still have dependencies to install, you will get an error message, but this is okay!
    - From the stack trace, identify the module, then use `pip install <dependency>`
    - keep generating errors by running `python3 app.py` every time, and resolve each missing dependency
- Ex:
```C
Traceback (most recent call last):
File "/home/user/repos/NexresGraphQL/app.py", line 1, in <module>
from resdb_driver import Resdb
File "/home/user/repos/NexresGraphQL/resdb_driver/__init__.py", line 1, in <module>
from .driver import Resdb
File "/home/user/repos/NexresGraphQL/resdb_driver/driver.py", line 4, in <module>
from .offchain import prepare_transaction, fulfill_transaction
File "/home/user/repos/NexresGraphQL/resdb_driver/offchain.py", line 8, in <module>
from .transaction import Input, Transaction, TransactionLink, _fulfillment_from_details
File "/home/user/repos/NexresGraphQL/resdb_driver/transaction.py", line 14, in <module>
import base58
ModuleNotFoundError: No module named 'base58'
```
- In the last line, you can run `pip install base58`
- Keep running `python3 app.py` until you get to the broken dependency (ex: pysha3, sha3), then try running `sudo apt-get install python3.10-dev`
- Hopefully, all your dependencies should be installed, and running `python3 app.py` will run the server!

Start the app using:

    > python3 app.py

This would be hosted on your local server to test the React App/Frontend.
Also, this would give info on the console about the POST command, which are used to create an account. This consists of 3 stages: publicKey. AES encrypted private key and decrypted privateKey

Note: To run Ubuntu virtual instance, open Command Prompt, then type `ubuntu2004` to open an instance
