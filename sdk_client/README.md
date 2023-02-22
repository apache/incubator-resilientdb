# NexRes and nexres_sdk connection

Connection between NEXRES and PythonSDK: 

Inorder to use the [nexres_sdk](https://github.com/msadoghi/nexres_sdk), we need to setup the crow services which will give us an endpoint from NexRes to nexres_sdk.

We can hit nexres endpoint using python sdk to commit,retireve and transfer assets as transactions to NexRes.

Start two terminals shells.

In first terminal:

## Clone the repo and change directory
`git clone https://github.com/msadoghi/nexres.git`

`cd nexres`

Install the dependencies for NEXRES

`chmod +x ./INSTALL.sh`

`./INSTALL.sh`

## Start the KVServer
`chmod +x ./example/start_kv_server.sh`

`./example/start_kv_server.sh`

NexRes has started. Now in a seperate terminal, we will start the endpoint service.

In second terminal:

### Build and Compile the CROW Service
`bazel build sdk_client/crow_service_main`

### Run binaries and start the CROW Service
`bazel-bin/sdk_client/crow_service_main example/kv_client_config.config sdk_client/server_config.config`

EXAMPLE OUTPUT:
```
(2022-12-19 06:12:02) [INFO    ] Crow/master server is running at http://0.0.0.0:18000 using 16 threads
(2022-12-19 06:12:02) [INFO    ] Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs
```

We will now use the http://0.0.0.0:18000 as `database url` in [nexres_sdk](https://github.com/msadoghi/nexres_sdk).

# NexRes on Cloud Platforms:

Instructions for setting a remote NexRes server, then, use nexres_sdk locally for development.

### On Local Machine
On local system, setup the nexres_sdk environment from the instructions provided in the [nexres_sdk repository](https://github.com/msadoghi/nexres_sdk).

### On Remote Machine

Inorder to setup Nexres instance on cloud, in your cloud VM perform all of the above steps, as a result, it will generate a NexRes server with an endpoint. Take a note of the output after crow service starts as the port number is mentioned there.

Use [remote port forwarding](https://en.wikipedia.org/wiki/Port_forwarding), then forward you remote port (in above case port 18000) to local machine.

The most convinient way to do this is by using Port option in [Remote-SSH](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh) extention of [VSCode](https://code.visualstudio.com/).

A quick and helpful tutorial on using Remote-SSH is [here](https://www.youtube.com/watch?v=7kum46SFIaY&t=202s).

You can also setup port forwarding using [linux commands](https://askubuntu.com/questions/311142/port-forward-in-terminal-only) or using [MobaXTerm](https://mobaxterm.mobatek.net/).

Now on your Local Machine, in the nexres_sdk code, the `database url` will be : ` localhost:18000`
