<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-->

---
layout: default
title: 'ResilientDB Configuration'
parent: 'ResilientDB'
nav_order: 8
---

# Chapter 8: ResilientDB Configuration

You've journeyed through the heart of ResilientDB! We started with how applications talk to the system ([Chapter 1: Client Interaction](01_client_interaction)), how messages travel ([Chapter 2: Network Communication](02_network_communication)), how agreement is reached ([Chapter 3: Consensus Management](03_consensus_management)), how messages are organized ([Chapter 4: Message/Transaction Collection](04_message_transaction_collection)), how transactions are executed ([Chapter 5: Transaction Execution](05_transaction_execution)), how data is stored ([Chapter 6: Storage Layer](06_storage_layer)), and how the system saves progress and recovers from crashes ([Chapter 7: Checkpointing & Recovery](07_checkpointing)).

Now, think about setting up a new ResilientDB network or connecting a client to an existing one. How does a replica know which other computers are part of the network? How does a client know where to send its requests? How does the system know which security keys to use, or how long to wait for consensus messages?

Welcome to the final chapter of our core concepts tour! We'll explore **ResilientDB Configuration**, managed primarily through the `ResDBConfig` object.

## Why Do We Need Configuration?

Imagine you're setting up a new computer game to play online with friends. Before you can start, you need to configure some settings:

- What's your username?
- What are the server addresses of your friends' games?
- What are the game rules (difficulty level, time limits)?

Similarly, before a ResilientDB node (replica) or client can start working, it needs initial setup instructions. It needs to know:

- Who are the other participants (replicas) in the network?
- How can I identify myself securely?
- What are the "rules of engagement" for communication and agreement?

This setup information is provided through **configuration**.

## Meet the Settings Menu: `ResDBConfig`

The `ResDBConfig` object acts like the central **settings menu** for a ResilientDB node or client. It holds all the crucial parameters needed to start up and participate correctly.

**Analogy:** Think of `ResDBConfig` as the sheet of instructions and addresses you'd give to a new team member joining a project. It tells them who else is on the team, their contact details, the project deadlines, and the procedures to follow.

Without this configuration, a node would be isolated and wouldn't know how to connect, communicate, or participate in the consensus process.

## What's Inside the Settings Menu (`ResDBConfig`)?

The `ResDBConfig` object bundles several key pieces of information:

1.  **Who are the participants? (`replicas_`, `self_info_`)**

    - It contains a list of `ReplicaInfo` objects. Each `ReplicaInfo` stores the network address (IP and port) and ID of one replica in the system.
    - It also stores the `ReplicaInfo` for the current node itself (`self_info_`). This tells the node its own identity and listening address.

    ```protobuf
    // Simplified from platform/proto/replica_info.proto
    message ReplicaInfo {
      int32 id = 1;       // Unique ID for the replica
      string ip = 2;      // IP address (e.g., "192.168.1.100")
      int32 port = 3;     // Port number (e.g., 12345)
      // CertificateInfo certificate_info = 4; // Public key/cert (optional)
    }
    ```

    - **Why?** This allows components like the [ReplicaCommunicator](02_network_communication__replicacommunicator___servicenetwork_.md) to know where to send messages and the [ServiceNetwork](02_network_communication__replicacommunicator___servicenetwork_.md) to know where to listen.

2.  **How do I identify myself securely? (`private_key_`, `public_key_cert_info_`)**

    - It holds the node's **private key** (`KeyInfo`). This is kept secret and used to digitally sign outgoing messages, proving they came from this node.
    - It often holds the node's **public key certificate** (`CertificateInfo`). This contains the public key (which corresponds to the private key) and information verifying the node's identity. This certificate can be shared so others can verify the node's signatures.

    ```protobuf
    // Simplified from common/proto/signature_info.proto
    message KeyInfo {
      bytes key = 1;         // The actual private key data
      enum Type { ... } type = 2; // Type of key (e.g., RSA, ED25519)
      // ... other fields ...
    }

    message CertificateInfo {
      // Contains PublicKeyInfo (ID, IP, Port, Public Key)
      // and potentially signatures from a Certificate Authority
      // ... details omitted ...
    }
    ```

    - **Why?** This is crucial for security. Signatures prevent others from impersonating a node and ensure message integrity, which is vital for consensus algorithms like PBFT ([Chapter 3](03_consensus_management__consensusmanager_.md)).

3.  **What are the rules of the game? (`config_data_`)**

    - This holds a `ResConfigData` object, which contains many operational parameters:
      - **Consensus Settings:** Timeouts for various phases (like view changes), batch sizes for transactions, number of worker threads.
      - **Fault Tolerance:** The configuration implicitly defines `f`, the maximum number of faulty replicas the system can tolerate. `ResDBConfig` calculates values like `2f+1` (`GetMinDataReceiveNum`) needed for PBFT voting thresholds.
      - **Feature Flags:** Enable/disable features like checkpointing ([Chapter 7](07_checkpointing___recovery__checkpointmanager___recovery_.md)), signature verification, performance monitoring.
      - **Checkpointing:** How often to take checkpoints (`checkpoint_water_mark_`), where to store logs (`checkpoint_logging_path_`).

    ```cpp
    // Simplified from platform/config/resdb_config.cpp
    int ResDBConfig::GetMinDataReceiveNum() const {
      // Calculate 'f', the max faulty nodes allowed
      int f = (replicas_.size() - 1) / 3;
      // PBFT requires 2f+1 votes for agreement (must be at least 1)
      return std::max(2 * f + 1, 1);
    }
    ```

    - **Why?** These parameters tune the performance, resilience, and behavior of the ResilientDB network. They define how the different components we've discussed should operate.

## Setting Up: Loading the Configuration

How does the `ResDBConfig` object get filled with all this information? Usually, it's loaded from files when a node or client starts up.

1.  **The Config Files:**

    - **Replica List (`config.config` or similar):** A simple text file listing the ID, IP, and port of each replica.
      ```
      # Example config.config
      0 127.0.0.1 12345
      1 127.0.0.1 12346
      2 127.0.0.1 12347
      3 127.0.0.1 12348
      ```
    - **Main Configuration (`config.json`):** A JSON file containing more detailed settings, including performance parameters, feature flags, and potentially replica info grouped by region.
      ```json
      // Example simplified config.json fragment
      {
        "self_region_id": 1,
        "region": [
          {
            "region_id": 1,
            "replica_info": [
              { "id": 0, "ip": "127.0.0.1", "port": 12345 },
              { "id": 1, "ip": "127.0.0.1", "port": 12346 },
              { "id": 2, "ip": "127.0.0.1", "port": 12347 },
              { "id": 3, "ip": "127.0.0.1", "port": 12348 }
            ]
          }
        ],
        "max_process_txn": 1024,
        "view_change_timeout_ms": 30000,
        "is_performance_running": false
      }
      ```
    - **Key/Certificate Files:** Separate binary files containing the node's private key (`private.key`) and certificate (`cert.cert`).

2.  **Helper Functions (`resdb_config_utils.cpp`):** ResilientDB provides utility functions to read these files and create the `ResDBConfig` object.

    ```cpp
    // Simplified from platform/config/resdb_config_utils.cpp

    // Reads the simple ID IP Port format
    std::vector<ReplicaInfo> ReadConfig(const std::string& file_name) {
      std::vector<ReplicaInfo> replicas;
      std::ifstream infile(file_name.c_str());
      int id; std::string ip; int port;
      while (infile >> id >> ip >> port) { // Read line by line
        ReplicaInfo info;
        info.set_id(id); info.set_ip(ip); info.set_port(port);
        replicas.push_back(info);
      }
      // ... error checking ...
      return replicas;
    }

    // Reads the JSON config file
    ResConfigData ReadConfigFromFile(const std::string& file_name) {
      std::ifstream infile(file_name.c_str());
      std::stringstream json_data;
      json_data << infile.rdbuf(); // Read whole file

      ResConfigData config_data; // The protobuf message to fill
      google::protobuf::util::JsonStringToMessage(json_data.str(), &config_data);
      // ... error checking ...
      return config_data;
    }

    // Reads key/cert files (simplified)
    KeyInfo ReadKey(const std::string& file_name) { /* ... reads binary file ... */ }
    CertificateInfo ReadCert(const std::string& file_name) { /* ... reads binary file ... */ }

    // Puts it all together
    std::unique_ptr<ResDBConfig> GenerateResDBConfig(
        const std::string& config_file, // JSON file
        const std::string& private_key_file,
        const std::string& cert_file) {

      ResConfigData config_data = ReadConfigFromFile(config_file);
      KeyInfo private_key = ReadKey(private_key_file);
      CertificateInfo cert_info = ReadCert(cert_file);

      // Extract self info (ID, IP, Port) from the certificate
      ReplicaInfo self_info;
      self_info.set_id(cert_info.public_key().public_key_info().node_id());
      self_info.set_ip(cert_info.public_key().public_key_info().ip());
      self_info.set_port(cert_info.public_key().public_key_info().port());
      *self_info.mutable_certificate_info() = cert_info; // Embed cert in self_info

      // Create the ResDBConfig object using a constructor
      return std::make_unique<ResDBConfig>(config_data, self_info,
                                          private_key, cert_info);
    }
    ```

    This code shows functions reading the different file types (text list, JSON, binary keys) and using the data to construct a `ResDBConfig` object.

## Using the Configuration

Once created, the `ResDBConfig` object is passed around to many other components when they are initialized.

- **Clients ([Chapter 1](01_client_interaction__kvclient___utxoclient___contractclient___transactionconstructor_.md)):**

  ```cpp
  // Simplified client creation
  #include "platform/config/resdb_config.h"
  #include "interface/kv/kv_client.h"

  ResDBConfig client_config = GenerateResDBConfig("config.config"); // Load replica list
  resdb::KVClient kv_client(client_config); // Pass config to client

  // kv_client now knows replica addresses from client_config
  ```

  The client uses `config.GetReplicaInfos()` to know where to send requests.

- **Network Components ([Chapter 2](02_network_communication__replicacommunicator___servicenetwork_.md)):**

  ```cpp
  // Simplified setup
  ResDBConfig node_config = GenerateResDBConfig(/* JSON, key, cert files */);

  // ReplicaCommunicator needs addresses of others
  ReplicaCommunicator comm(node_config, /* ... */);

  // ServiceNetwork needs own listening address
  ServiceNetwork service(node_config, /* service handler */);
  service.Run();
  ```

  `ReplicaCommunicator` uses `node_config.GetReplicaInfos()` and `ServiceNetwork` uses `node_config.GetSelfInfo()`.

- **Consensus ([Chapter 3](03_consensus_management__consensusmanager_.md)):**

  ```cpp
  // Simplified setup
  ConsensusManagerPBFT consensus_manager(node_config, /* executor, etc. */);

  // Inside consensus logic...
  int required_votes = node_config.GetMinDataReceiveNum(); // Gets 2f+1
  // Check if enough votes received...
  ```

  The `ConsensusManager` uses the config to get crucial values like the number of required votes (`2f+1`) and timeouts.

The `ResDBConfig` object acts as a read-only container passed to components needing setup parameters.

## Code Glimpse: The `ResDBConfig` Class

Let's look at the structure of the class itself.

```cpp
// Simplified from platform/config/resdb_config.h
namespace resdb {

class ResDBConfig {
 public:
  // Constructor taking detailed data (often called by GenerateResDBConfig)
  ResDBConfig(const ResConfigData& config_data, const ReplicaInfo& self_info,
              const KeyInfo& private_key,
              const CertificateInfo& public_key_cert_info);

  // --- Getters for the stored information ---

  // Get the list of all replica addresses/IDs
  const std::vector<ReplicaInfo>& GetReplicaInfos() const;

  // Get this node's own address/ID/certificate
  const ReplicaInfo& GetSelfInfo() const;

  // Get this node's secret private key
  KeyInfo GetPrivateKey() const;

  // Get the detailed parameters from ResConfigData
  ResConfigData GetConfigData() const;

  // --- Calculated Values ---

  // Get total number of replicas (N)
  size_t GetReplicaNum() const;

  // Get minimum votes needed for PBFT agreement (2f+1)
  int GetMinDataReceiveNum() const;

  // Get minimum votes needed for client responses (f+1)
  int GetMinClientReceiveNum() const;

  // Get max faulty replicas tolerated (f)
  size_t GetMaxMaliciousReplicaNum() const;

  // --- Other settings ---
  int GetClientTimeoutMs() const;
  int GetCheckPointWaterMark() const;
  // ... many more getters for timeouts, flags, paths, etc. ...

 private:
  // The actual data members storing the configuration
  ResConfigData config_data_;
  std::vector<ReplicaInfo> replicas_;
  ReplicaInfo self_info_;
  KeyInfo private_key_;
  CertificateInfo public_key_cert_info_;
  // ... other member variables for settings not in ResConfigData ...
};

} // namespace resdb
```

The header shows the constructors used to create the object and various `Get...` methods that components call to retrieve the configuration values they need. The actual data is stored in private member variables.

```cpp
// Simplified constructor from platform/config/resdb_config.cpp
ResDBConfig::ResDBConfig(const ResConfigData& config_data,
                         const ReplicaInfo& self_info,
                         const KeyInfo& private_key,
                         const CertificateInfo& public_key_cert_info)
    : config_data_(config_data),      // Copy the ResConfigData parameters
      self_info_(self_info),          // Copy this node's info
      private_key_(private_key),      // Copy the private key
      public_key_cert_info_(public_key_cert_info) // Copy the certificate
{
  // Extract the replica list from the ResConfigData based on self_region_id
  replicas_.clear();
  for (const auto& region : config_data.region()) {
    if (region.region_id() == config_data.self_region_id()) {
      for (const auto& replica : region.replica_info()) {
        replicas_.push_back(replica); // Add replicas from my region
      }
      break;
    }
  }
  // Set default values for some parameters if they weren't in config_data
  if (config_data_.view_change_timeout_ms() == 0) {
    config_data_.set_view_change_timeout_ms(/* default value */);
  }
  // ... set other defaults ...
}
```

The constructor initializes the member variables from the passed-in data. It also handles extracting the correct replica list from the potentially region-based `ResConfigData` and sets sensible defaults for parameters if they are missing.

## Conclusion

You've now learned about `ResDBConfig`, the essential **settings menu** for ResilientDB.

- It holds crucial setup information: **network addresses** (`ReplicaInfo`), **security keys** (`KeyInfo`, `CertificateInfo`), and **operational parameters** (`ResConfigData`).
- It's typically loaded from **configuration files** (text, JSON, binary keys) using helper functions when a node or client starts.
- The `ResDBConfig` object is passed to various components during initialization, providing them with the necessary parameters to function correctly within the ResilientDB network.

This concludes our tour of the core concepts behind `incubator-resilientdb`. From sending a client request to configuring the entire network, you now have a foundational understanding of how the main pieces fit together. While each component has much more depth, this overview should provide a solid starting point for further exploration or contribution!

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)
