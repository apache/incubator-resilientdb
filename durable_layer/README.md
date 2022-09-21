# Durable layer
## How to use
Look at [proto/durable.proto](https://github.com/msadoghi/nexres/blob/master/proto/durable.proto) and [proto/replica_info.proto](https://github.com/msadoghi/nexres/blob/master/proto/replica_info.proto) for how to format the durability settings.

A config file for 4 replicas on the local machine can be found at [config/example/kv_config.config](https://github.com/msadoghi/nexres/blob/master/example/kv_config.config).

## Warning

If "path" is not set in the durability settings for RocksDB or LevelDB then default paths will be generated in the /tmp/ folder. If you are testing Nexres
on your local machine using localhost ip, then do not set custom paths, because multiple processes are not allowed to open up RocksDB/LevelDB at the same
time. When no custom paths are set, the durable layer uses the cert file names to generate separate directories for each port of the localhost ip.
