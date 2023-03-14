# Dissecting BFT Consensus: In Trusted Components we Trust! 

In this document, we explain how to run this artifact of our Eurosys'23 [paper](https://doi.org/10.1145/3552326.3587455) and reproduce the presented results.


## Setup code and machines
The code is available at [link](https://github.com/resilientdb/resilientdb/tree/resilientdb-legacy-eurosys23). This is a branch of [ResilientDB](https://resilientdb.com/) version **V3** (prior to the release of version **NexRes**). In order to reproduce the results presented in the paper, you would need the following:
- A development machine to build the codebase and to run necessary deployment and execution scripts (**Dev-Machine**).
- A set of machines to run replicas and clients (we use the terms server and replica interchangeably). 

The dev-machine needs to have access to the source code, resilientdb dependencies, and SGX dependencies. 
If you have access to Oracle Cloud Infrastructure (OCI) machines, we provide ready-to-use images for both dev-machine and replica. Later in this document, we explain how to use these images. You also can setup the code and dependencies on your machine or the cloud provider of your choice, which may necessitate specific installation. 

**Note**: All developement and experimentation was done on Ubuntu 18.04.6 LTS.

### Setup without OCI images
1. Clone the code base from this [Repo](https://github.com/resilientdb/resilientdb/tree/resilientdb-legacy-eurosys23)
2. Unzip all the dependencies using this command 
    ```
    cd deps; \ls | xargs -n 1 tar -xvf
    ```
3. Install Intel SGX dependencies from [Here](https://github.com/intel/linux-sgx): this requires building and installing 
4. For the server (replica) machines, you need to only install SGX dependencies and have a directory called `resilientdb` in home directory
### Setup with OCI images
For this, you need to have access to an OCI console. Following steps state how to create dev-machine and server machines:
 


 1. Create images from the above links. [Oracle guide](https://docs.oracle.com/en-us/iaas/Content/Compute/Tasks/imageimportexport.htm#Importing). The following are Object Storage URLs for various machines:
    - dev-machine:      [Image-Link](https://objectstorage.us-phoenix-1.oraclecloud.com/p/jGPwGQ_jBBTtbtnKBSrP5EUoOE1HRSI2Q3WAWzdJ2F42lvMSZ9EcbbhmA0AYPwcM/n/ax8oq4eg8tc3/b/expo_bucket/o/eurosys-dev-image)
    - server:  [Image-Link](https://objectstorage.us-phoenix-1.oraclecloud.com/p/UTEX-ZOq5ovW31Inn_1yrTLS7hW9Tj4Gx4Hhx7Sfpq_a42p8PA2SgrUzWsSKyIwM/n/ax8oq4eg8tc3/b/expo_bucket/o/eurosys-machine-image)
2. Create one dev-machine and as many servers as you need (based on the scale of experiments that you wish to reproduce) using [Oracle guide](https://docs.oracle.com/en-us/iaas/Content/Compute/Tasks/launchinginstance.htm#linux__linux-create)


---

## Steps to Run and Compile 
* Create an **obj** directory inside **resilientdb** directory, to store object files, and **results** directory to store the results.

        mkdir obj
        mkdir results
        
* We provide a script **startResilientDB.sh** to compile and run the code. To run **ResilientDB** on a cloud provider such as AWS, OCI, or Google Cloud, you need to specify the **Private IP Addresses** of each replica. 
* The code will be compiled on the machine that runs the script **startResilientDB.sh** (in our case dev-machine) and sends the binary files over SSH to the **resilientdb** folder in all the other nodes (servers and clients). The directory which contains the **resilientdb** in each node should be set as ``home_directory`` in the following files:
    1. scripts/scp_binaries.sh
    2. scripts/scp_results.sh
    3. scripts/simRun.py
* **Modify the ``CNODES`` and ``SNODES`` arrays in ``scripts/startResilientDB.sh`` with the IP Addresses of clients and servers, respectively.**
* Adjust the necessary parameters in the configuration file ``config.h``, such as the number of clients and servers.
* Run script as: 
```
    ./scripts/startResilientDB.sh <number of servers> <number of clients> <result name>

# for example:
    ./scripts/startResilientDB.sh 4 1 PBFT 
```
In the above example, the `startResilientDB.sh` scripts needs to have 4 IP addresses in `SNODES` and 1 IP address in `CNODES`.

* Post running the script, all the results will be stored inside the **results** folder.


#### What is happening behind the scenes?

* The code is compiled using the command: **make clean; make**
* Post compilation, two new files are created: **runcl** and **rundb**.
* Each machine that is going to act as a client needs to execute **runcl**.
* Each machine that is going to act as a replica needs to execute **rundb**. 
* The script runs each binary as: **./rundb -nid\<numeric identifier\>**
* This numeric identifier starts from **0** (for the primary) and increases as **1,2,3...** for subsequent replicas and clients.


---

## Configurations for different Protocols

Before running `startResilientDB.sh` script, you need to adjust the `config.h` file to set necessary configuration parameters for the desired protocol/experiment. Following are some of the important parameters that may change in different experiments:
 - **NODE_CNT**: which refers to the number of replicas. 
 - **CLIENT_NODE_CNT**: which refers to the number of clients (for most of the experiments, we set it to 8). 
 - **SGX**: in order to run any protocol that uses hardware assist, you need to set this to true (all protocols except PBFT and Zyzzyva).
 - **LOCAL_FAULT**: set it to true for running failure experiments for protocols stated in the paper.
 - **TIMER_ON**: set it to true to start the timer for failure experiments.
 - The following table shows which parameter to set to true for each protocol: 

| Syntax      | Description |
| ----------- | ----------- |
| **Protocol Name**  | **Config File Parameter** |
| PBFT-EA            | A2M                       |
| MinBFT             | MinBFT                    |
| MinZZ              | MinZZ                     |
| OPBFT-EA           | CONTRAST + A2M            |
| Flexi-BFT          | FLEXI_PBFT                |
| Flexi-ZZ           | FLEXI_ZZ                  |
| PBFT               | PBFT                      |
| ZYZZYVA            | ZYZZYVA                   |

### Sample Config Files

To help identify which parameters need to be changed for running a specific protocol, we have created a directory ``sample_config`` that includes the sample configuration files for each protocol. These files can be used to run specific protocols, and the user needs to only reset the number of servers and clients as needed.

### Result Interpretation
To present average throughput and latency of replicas, for each run of an experiment, in the directory ``script``, we provide access to scripts ``results.sh`` and ``colorized_results.sh``.

---

## Contact:

Please contact Sajjad Rahnama (srahnama@ucdavis.edu) and Suyash Gupta (suyash.gupta@berkeley.edu) if you have any questions or need any help in installation or executing the codebase.
