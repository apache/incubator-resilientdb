# resdb_rust_sdk
Rust SDK for ResilientDB

### Setup
To start the dev environment on your system, run the following command: 
`docker buildx build --platform=linux/amd64 -t sdkimage:latest .`
- This command will create a docker container named `sdkimage`

To run the container, execute the following command after:
`docker run -it --platform linux/amd64 sdkimage:latest`
