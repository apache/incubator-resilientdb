![](https://img.shields.io/badge/language-c++-orange.svg)
![](https://img.shields.io/badge/platform-Ubuntu20.0+-lightgrey.svg)
![GitHub](https://img.shields.io/github/license/resilientdb/resilientdb)
![](https://github.com/msadoghi/nexres/blob/image-data/badge.svg)
![build](https://github.com/msadoghi/nexres/workflows/bazel-build%20CI/badge.svg)
![build](https://github.com/msadoghi/nexres/workflows/Unite%20Test/badge.svg)



This repo runs Thunderbolt on SMALLBANK


## Online Documentation:

You can find the latest ResilientDB documentation, including a programming guide, on the **[project web blog](https://blog.resilientdb.com/archive.html?tag=NexRes)**. This README file only contains basic setup instructions.

![Nexres](./img/nexres.pdf)

## OS Requirements
Ubuntu 20+

---

## Steps to run ResilientDB

Install dependencies:

    ./INSTALL.sh

Setup your test environment:

	Go to scripts/deploy: cd ./scripts/deploy
	Write down your nodes IPs, including the ssh key on config/kv_performance_server.conf
	The IPs include X replicas + 4 client nodes. So if there are 8 replicas, 4 of them are the clients used to propose transactions.

Start the experiments:

	./performance/tuskcc_performance.sh config/kv_performance_server.conf


The results will be shown once the experiments are done.

