# ResilientDB GraphQL
ResilientDB GraphQL Server

# Get Prepared 

(If you're using this for the first time, the steps below would be useful)

Install **Ubuntu 20.04** on your local machine.

Once installed, go to File Explorer -> Linux -> Ubuntu 20.04.

Clone this repository.

Install python3 (version - 3.9+) and ensure pip is installed using the command.

    sudo apt-get install python3-pip

Also make sure to install the venv module which creates a virtual Python environment that helps encapsulate the project's dependencies and prevents possible conflicts with the global Python environment. The command is:

    sudo apt-get install -y python3.10-venv

Create a virtual environment:

    python3 -m venv venv

Start a virtual environment:

    source venv/bin/activate

Go back from the virtual environment when you no longer need it:

    deactivate

# Using the SDK 

To use the SDK, you need to start a KV service first, which you can refer to the [resilientdb](https://github.com/resilientdb/resilientdb) repository and the [blog](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html). 

Then you should start the crow http service, which may take a few minutes at the first time.

    bazel build service/http_server:crow_service_main

    bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config

After starting the crow service, you can optionally test it by using the **curl** command to send HTTP requests.

    curl -X POST -d '{"id":"key1","value":"value1"}' 127.0.0.1:18000/v1/transactions/commit

    curl 127.0.0.1:18000/v1/transactions/key1

The expected output of the two commands above are: `id: key1` and `{"id":"key1","value":"value1"}`, respectively.

Start your virtual environment.

    source venv/bin/activate

Run the requirements.txt command to install the SDK related dependencies.

    pip3 install -r requirements.txt

Try the test script.

    python3 test_sdk.py

You have successfully run the script if you see the output: `The retrieved txn is successfully validated`

# GraphQL

<!-- 7. (Temporary resolve) Copy app.py to the nexres_sdk folder to ensure execution -->

(Optional) If there is an error while running any command relating to NexRes, this command below may help:

    sudo apt-get install python3.10-distutils

Note: If there is an error with the pip version, use the command:

    curl -sS https://bootstrap.pypa.io/get-pip.py | python3.10

Open a venv in the Ubuntu server and run the command: 

    source venv/bin/activate

    sudo apt-get install cloud-init

Install the remaining dependencies in venv

    python3 -m pip install strawberry-graphql

    python3 -m pip install flask_cors

**BUG FIX**: in the case requirements.txt doesn't work, and the `cloud-init` version is not found, edit the file to be:
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

    python3 app.py

This would be hosted on your local server to test the React App/Frontend.
Also, this would give info on the console about the POST command, which are used to create an account. This consists of 3 stages: publicKey. AES encrypted private key and decrypted privateKey

Note: To run Ubuntu virtual instance, open Command Prompt, then type `ubuntu2004` to open an instance

# Serve GraphQL Using Nginx

Install the Nginx web server package:

    sudo apt-get install nginx -y

Once the Nginx is installed, start and enable the Nginx service using the following command:

    systemctl start nginx
    systemctl enable nginx

Activate your virtual environment and install Gunicorn and Flask:

    source venv/bin/activate
    pip install wheel
    pip install gunicorn flask

Verify whether Gunicorn can serve the application correctly using the command below:

    gunicorn --bind 0.0.0.0:5000 wsgi:app

If everything is fine, you should get the following output:

    [2023-10-06 22:28:11 +0000] [24074] [INFO] Starting gunicorn 21.2.0
    [2023-10-06 22:28:11 +0000] [24074] [INFO] Listening at: http://0.0.0.0:5000 (24074)
    [2023-10-06 22:28:11 +0000] [24074] [INFO] Using worker: sync
    [2023-10-06 22:28:11 +0000] [24075] [INFO] Booting worker with pid: 24075

Press CTRL+C to stop the application. And deactivate the virtual environment.

    deactivate

Next, you will need to create a systemd unit file for the Flask application:

    sudo vim /etc/systemd/system/flask.service

Add the following lines:

    [Unit]
    Description=Gunicorn instance to serve Flask
    After=network.target
    [Service]
    User=root
    Group=www-data
    WorkingDirectory=\{Your Absolute Path to this Folder\}
    Environment="PATH=\{Your Absolute Path to this Folder\}/venv/bin"
    ExecStart=\{Your Absolute Path to this Folder\}/venv/bin/gunicorn --bind 0.0.0.0:8000 wsgi:app
    [Install]
    WantedBy=multi-user.target

Next, reload the systemd daemon with the following command:

    sudo systemctl daemon-reload

Next, start the flask service and enable it to start at system reboot:

    sudo systemctl start flask
    sudo systemctl enable flask

Next, verify the status of the flask with the following command:

    sudo systemctl status flask

Output:

    ● flask.service - Gunicorn instance to serve Flask
     Loaded: loaded (/etc/systemd/system/flask.service; enabled; vendor preset: enabled)
     Active: active (running) since Fri 2023-10-06 22:20:00 UTC; 14min ago
    Main PID: 23745 (gunicorn)
      Tasks: 2 (limit: 38067)
     Memory: 40.6M
     CGroup: /system.slice/flask.service
             ├─23745 /home/ubuntu/ResilientDB-GraphQL/venv/bin/python3 /home/ubuntu/ResilientDB-GraphQL/venv/b>
             └─23747 /home/ubuntu/ResilientDB-GraphQL/venv/bin/python3 /home/ubuntu/ResilientDB-GraphQL/venv/b>


Next, you will need to configure Nginx as a reverse proxy to serve the Flask application through port 80. To do so, create an Nginx virtual host configuration file:

    sudo vim /etc/nginx/conf.d/flask.conf

Add the following lines:

    server {
        listen 80;
        server_name {The public IP address of your machine};
        location / {
            proxy_pass http://127.0.0.1:8000;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
        }
    }

Save and close the file then verify the Nginx for any syntax error:

    sudo nginx -t

You should see the following output:

    nginx: the configuration file /etc/nginx/nginx.conf syntax is ok
    nginx: configuration file /etc/nginx/nginx.conf test is successful

Finally, restart the Nginx service to apply the changes:

    sudo systemctl restart nginx

Then, test with:

    {The public IP address of your machine}/graphql