# ResilientDB Rust SDK

Rust Software Developement Kit that allows Rust users to interface with [ResilientDB](https://github.com/resilientdb/resilientdb). The SDK is designed so that Rust users can develop robust blockchain applications using ExpoLab's ResilientDB. 

## Motivation


## Usage
Currently the SDK is able to fetch transactions from a database instance. 

### Our dev environement setup
The repository includes a dockerfile that can be used to build a container.

To start the dev environment on your system, run the following command:  

`docker buildx build --platform=linux/amd64 -t sdkimage:latest .`  

- This command will create a docker container named `sdkimage`

To run the container, execute the following command after:  

`docker run -it --platform linux/amd64 sdkimage:latest`


