# ResilientDB Rust SDK

![Build](https://github.com/dhruvsangamwar/resilientdb_rust_sdk/actions/workflows/rust.yml/badge.svg)
[![Crates.io](https://img.shields.io/crates/v/resilientdb_rust_sdk)](https://crates.io/crates/resilientdb_rust_sdk)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)


## Overview

The ResilientDB Rust SDK is a powerful Rust library that facilitates seamless interaction with resource databases. It offers comprehensive APIs for managing transactions and blocks, simplifying integration with systems requiring robust data management capabilities.

## Features

- **Transaction Management**: Create, retrieve, and manage transactions effortlessly.
- **Block Operations**: Retrieve, group, and query information about blocks based on specified criteria.
- **Flexible Configuration**: Tailor your interactions with resource databases using versatile configuration options.

## Installation

Add this line to your `Cargo.toml` file to integrate the SDK into your Rust project:

```toml
[dependencies]
resilientdb_rust_sdk = "0.1.0"
```

## Usage

```rust
// Import the ResDB SDK
use resilientdb_rust_sdk::ResDB;

// Create a new ResDB instance
let res_db = ResDB::new();

// Example: Create a new transaction object
let transaction = res_db.create_object::<YourTransactionType>();

// Example: Get all transactions from a specified API endpoint
let all_transactions = res_db.get_all_transactions::<YourTransactionType>("https://api.example.com").await;
```

## Examples

```rust
// Example: Retrieve all blocks from a specified API endpoint
let all_blocks = res_db.get_all_blocks::<T>("https://api.example.com/blocks").await;

// Example: Group blocks with a specified batch size
let grouped_blocks = res_db.get_blocks_grouped::<T>("https://api.example.com/blocks", &100).await;
```

<!-- ## Documentation

For detailed information about the SDK's API and usage, refer to the [official documentation](https://your-crate-docs-url). -->

## Contributing

Contributions are welcome! If you encounter any issues or have suggestions for improvements, please open an issue or submit a pull request.

## License

This SDK is licensed under the [Apache-2.0 License](https://opensource.org/licenses/Apache-2.0).

## Acknowledgments

- Special thanks to [contributors](https://github.com/dhruvsangamwar/resilientdb_rust_sdk/graphs/contributors).
- This SDK leverages the power of [serde](https://crates.io/crates/serde) for serialization and deserialization.
