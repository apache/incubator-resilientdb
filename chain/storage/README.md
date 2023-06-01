# Durable layer
## How to use
Look at [proto/durable.proto](https://github.com/msadoghi/nexres/blob/master/proto/durable.proto) and [proto/replica_info.proto](https://github.com/msadoghi/nexres/blob/master/proto/replica_info.proto) for how to format the durability settings.

A config file for 4 replicas on localhost can be found at [config/example/kv_config.config](https://github.com/msadoghi/nexres/blob/master/example/kv_config.config).

## Warning

If "path" is not set in the durability settings for RocksDB or LevelDB then default paths will be generated in the /tmp/ folder, which is cleared whenever the machine is shut down. 

If you are testing Nexres on your local machine using localhost ip, then make sure to set generate_unique_pathnames to true, because multiple processes are not allowed to open up the same RocksDB/LevelDB directory at the same time. When generate_unique_pathnames is set to true, the durable layer uses the cert file names to generate separate directories for each port of the localhost ip.

For example, the process hosting a server with cert_1.cert will append "1" to its directory path. If the cert file name does not contain a number and generate_unique_pathnames is set, "0" will be appended to the path.
