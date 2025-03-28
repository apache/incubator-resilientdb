/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

// Imports
use crate::blocks;
use crate::crypto;
use crate::transaction;

use openssl::pkey::PKey;
use openssl::pkey::Private;
use openssl::rsa::Rsa;

use serde::Serialize;
use serde_json::Value;
use std::collections::HashMap;

/// A struct representing the resource database.
pub struct ResDB;

/// ResDB is a struct is the entrypoint for the APIs within it.
///
/// It provides various APIs for interacting with transactions and blocks.
/// The ResDB struct can be instantiated using the `new` function.
///
/// # Examples
///
/// ```
/// use resdb::ResDB;
///
/// let res_db = ResDB::new();
/// ```
/// ResDB is a struct that represents a resilient database.
/// It provides various APIs for interacting with transactions and blocks.
/// The ResDB struct can be instantiated using the `new` function.

impl ResDB {
    /// Constructor for ResDB.
    pub fn new() -> Self {
        ResDB
    }

    /// Create a new object of a specified type using its default constructor.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of object to create.
    ///
    /// # Returns
    ///
    /// The created object of type `T`.
    pub fn create_object<T>(&self) -> T
    where
        T: Default,
    {
        T::default()
    }

    /// Get all transactions of a specified type from the given API endpoint.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of transactions to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of transactions of type `T` or an error of type `anyhow::Error`.
    pub async fn get_all_transactions<T>(&self, api_url: &str) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        let api_url = format!("{}/{}/{}", api_url, "v1", "transactions");
        transaction::get_all_transactions(&api_url).await
    }

    /// Get all transactions of a specified type from the given API endpoint and map.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of HashMaps with String keys and Value values,
    /// representing transactions of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_all_transactions_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}", api_url, "v1", "transactions");
        transaction::get_all_transactions_map(&api_url, map).await
    }

    /// Get transactions of a certain ID from the given API endpoint.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `id`: The ID of the transaction to retrieve.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of transactions to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of transactions of type `T` or an error of type `anyhow::Error`.
    pub async fn get_transaction_by_id<T>(
        &self,
        api_url: &str,
        id: &str,
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        let api_url = format!("{}/{}/{}/{}", api_url, "v1", "transactions", id);
        transaction::get_transaction_by_id(&api_url, id).await
    }

    /// Get transactions of a certain ID from the given API endpoint and map.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `id`: The ID of the transaction to retrieve.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of HashMaps with String keys and Value values,
    /// representing transactions of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_transaction_by_id_map(
        &self,
        api_url: &str,
        id: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}/{}", api_url, "v1", "transactions", id);
        transaction::get_transaction_by_id_map(&api_url, id, map).await
    }

    /// Get transactions of a specified type within a key range from the given API endpoint.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `key1`: The starting key of the range.
    /// - `key2`: The ending key of the range.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of transactions to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of transactions of type `T` or an error of type `anyhow::Error`.
    pub async fn get_transaction_by_key_range<T>(
        &self,
        api_url: &str,
        key1: &str,
        key2: &str,
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        let api_url = format!("{}/{}/{}/{}/{}", api_url, "v1", "transactions", key1, key2);
        transaction::get_transaction_by_key_range(&api_url, key1, key2).await
    }

    /// Get transactions within a key range from the given API endpoint and map.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `key1`: The starting key of the range.
    /// - `key2`: The ending key of the range.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of HashMaps with String keys and Value values,
    /// representing transactions of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_transaction_by_key_range_map(
        &self,
        api_url: &str,
        key1: &str,
        key2: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}/{}/{}", api_url, "v1", "transactions", key1, key2);
        transaction::get_transaction_by_key_range_map(&api_url, key1, key2, map).await
    }

    /// Post a transaction string to the specified API endpoint.
    ///
    /// # Parameters
    ///
    /// - `data`: The transaction data as a string.
    /// - `endpoint`: The URL of the API endpoint.
    ///
    /// # Returns
    ///
    /// A `Result` containing the response as a string or an error of type `anyhow::Error`.
    pub async fn post_transaction_string(
        &self,
        data: &str,
        endpoint: &str,
    ) -> Result<String, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}", endpoint, "graphql");
        transaction::post_transaction_string(data, &endpoint).await
    }

    /// Get all blocks of a specified type from the given API endpoint.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of blocks to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of blocks of type `T` or an error of type `anyhow::Error`.
    pub async fn get_all_blocks<T>(&self, api_url: &str) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        let api_url = format!("{}/{}/{}", api_url, "v1", "blocks");
        blocks::get_all_blocks(&api_url).await
    }

    /// Get all blocks of a specified type from the given API endpoint with additional parameters.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of HashMaps with String keys and Value values,
    /// representing blocks of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_all_blocks_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}", api_url, "v1", "blocks");
        blocks::get_all_blocks_map(&api_url, map).await
    }

    /// Get all blocks of a specified type from the given API endpoint, grouped by batches.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `batch_size`: The size of each batch.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of blocks to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of vectors of blocks of type `T` or an error of type `anyhow::Error`.
    pub async fn get_blocks_grouped<T>(
        &self,
        api_url: &str,
        batch_size: &i64,
    ) -> Result<Vec<Vec<T>>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned + std::default::Default,
    {
        let api_url = format!("{}/{}/{}/{}", api_url, "v1","blocks", batch_size);
        blocks::get_all_blocks_grouped(&api_url, batch_size).await
    }

    /// Get all blocks of a specified type from the given API endpoint, grouped by batches and additional parameters.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `batch_size`: The size of each batch.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of vectors of HashMaps with String keys and Value values,
    /// representing blocks of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_blocks_grouped_map(
        &self,
        api_url: &str,
        batch_size: &i64,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<Vec<HashMap<String, Value>>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}/{}", api_url, "v1", "blocks", batch_size);
        blocks::get_blocks_grouped_map(&api_url, batch_size, map).await
    }

    /// Get all blocks of a specified type from the given API endpoint within a specified range.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `range_begin`: The beginning of the range.
    /// - `range_end`: The end of the range.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of blocks to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of blocks of type `T` or an error of type `anyhow::Error`.
    pub async fn get_blocks_by_range<T>(
        &self,
        api_url: &str,
        range_begin: &i64,
        range_end: &i64,
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        let api_url = format!("{}/{}/{}/{}/{}", api_url, "v1" , "blocks", range_begin, range_end);
        blocks::get_blocks_by_range(&api_url, range_begin, range_end).await
    }

    /// Get all blocks of a specified type from the given API endpoint within a specified range, with additional parameters.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    /// - `range_begin`: The beginning of the range.
    /// - `range_end`: The end of the range.
    /// - `map`: A HashMap containing key-value pairs for additional parameters.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of HashMaps with String keys and Value values,
    /// representing blocks of type `T`, or an error of type `anyhow::Error`.
    pub async fn get_blocks_by_range_map(
        &self,
        api_url: &str,
        range_begin: &i64,
        range_end: &i64,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where 
    {
        let api_url = format!("{}/{}/{}/{}/{}", api_url, "v1", "blocks", range_begin, range_end);
        blocks::get_blocks_by_range_map(&api_url, range_begin, range_end, map).await
    }

    /// Generate a key pair for cryptographic operations.
    ///
    /// # Returns
    ///
    /// A tuple containing the public key and private key as strings.
    pub fn generate_keypair(&self) -> (String, String)
    where 
    {
        crypto::generate_keypair()
    }

    /// Get all blocks of a specified type from the given API endpoint.
    ///
    /// # Parameters
    ///
    /// - `api_url`: The URL of the API endpoint.
    ///
    /// # Generic Parameters
    ///
    /// - `T`: The type of blocks to retrieve.
    ///
    /// # Returns
    ///
    /// A `Result` containing a vector of blocks of type `T` or an error of type `anyhow::Error`.
    pub fn hash_data(&self, data: &str) -> String
    where 
    {
        crypto::hash_data(data)
    }

}
