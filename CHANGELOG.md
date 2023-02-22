# Change Log

### NexRes v1.3.0 ([2022-02-22](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.3.0))

**Implemented Enhancements:** 
* Added Python SDK that supports UTXO-like transactions such as asset creation, transfer, and multi-party validations. ([Arindaam Roy](https://github.com/royari), [Glenn Chen](https://github.com/glenn-chen), and [Julieta Duarte](https://github.com/juduarte00))
* Added REST CROW endpoints to interface with the on-chain KV service. ([Glenn Chen](https://github.com/glenn-chen) and [Julieta Duarte](https://github.com/juduarte00))
* Extended KV service range queries. ([Glenn Chen](https://github.com/glenn-chen) and [Julieta Duarte](https://github.com/juduarte00))

**Fixed Bugs**
* Fixed KV service queries on all values retrieving empty placeholder values ([Glenn Chen](https://github.com/glenn-chen))

### v2.0 ([2020-02-29](https://github.com/resilientdb/resilientdb/releases/tag/v2.0))

**Implemented Enhancements:** 
* A GUI display for ResilientDB to ease user interaction and analysis. [#9](https://github.com/resilientdb/resilientdb/issues/9) ([Sajjad Rahnama](https://github.com/sajjadrahnama) and [RohanSogani](https://github.com/RohanSogani))
* Added support for Smart Contracts with a Banking use case. [#7](https://github.com/resilientdb/resilientdb/issues/7) ([Sajjad Rahnama](https://github.com/sajjadrahnama))
* Monitoring results for visualizing graphs using influxdb. [#8](https://github.com/resilientdb/resilientdb/issues/8) ([Sajjad Rahnama](https://github.com/sajjadrahnama), [Dhruv Krishnan](https://github.com/DhruvKrish) and [Priya Holani](https://github.com/Holani))
* Added support for SQLite and provided a new representation for in-memory storage. [#6](https://github.com/resilientdb/resilientdb/issues/6) ([Sajjad Rahnama](https://github.com/sajjadrahnama))

**Fixed Bugs:**
* Bug in the Chain Initialization [#5](https://github.com/resilientdb/resilientdb/issues/5) ([Sajjad Rahnama](https://github.com/sajjadrahnama))


### v1.1 ([2019-12-05](https://github.com/resilientdb/resilientdb/releases/tag/v1.1)) 

**Implemented Enhancements:**
* Defined class to represent ledger. [6b0fdc5](https://github.com/resilientdb/resilientdb/commit/56f500fe5e4749c45f57dc8e62d12bc7a218ce69) ([gupta-suyash](https://github.com/gupta-suyash))

**Fixed Bugs:**
* Docker script fails to run for multiple client [#1](https://github.com/resilientdb/resilientdb/issues/1) ([RohanSogani](https://github.com/RohanSogani))	

