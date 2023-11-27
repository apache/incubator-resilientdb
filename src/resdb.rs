// resdb.rs

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

// Imports
use crate::transaction;
use crate::blocks;
use std::collections::HashMap;
use serde_json::Value;

// Define a public struct ResDB.
pub struct ResDB;

impl ResDB {
    // Constructor for ResDB.
    pub fn new() -> Self {
        ResDB
    }

    /** APIs provided for the transaction endpoint **/
    /** Functions that accept structs **/

    // Create a new object of a specified type using its default constructor.
    pub fn create_object<T>(&self) -> T
    where
        T: Default,
    {
        T::default()
    }

    // Get all transactions of a specified type from the given API endpoint.
    pub async fn get_all_transactions<T>(&self, 
        api_url: &str
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        transaction::get_all_transactions(api_url).await
    }

    // Get all transactions with additional parameters using a HashMap.
    pub async fn get_all_transactions_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        transaction::get_all_transactions_map(api_url, map).await
    }

    // Get a specific transaction by ID.
    pub async fn get_transaction_by_id<T>(
        &self,
        api_url: &str,
        id: &str,
    ) -> Result<T, anyhow::Error>
    where
        T: serde::de::DeserializeOwned + std::default::Default,
    {
        transaction::get_transaction_by_id(api_url, id).await
    }

    // Get a specific transaction by ID with additional parameters using a HashMap.
    pub async fn get_transaction_by_id_map(
        &self,
        api_url: &str,
        id: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        transaction::get_transaction_by_id_map(api_url, id, map).await
    }

    // Get transactions within a specified key range.
    pub async fn get_transaction_by_key_range<T>(
        &self,
        api_url: &str,
        key1: &str,
        key2: &str,
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        transaction::get_transaction_by_key_range(api_url, key1, key2).await
    }

     // Get transactions within a specified key range with additional parameters using a HashMap.
    pub async fn get_transaction_by_key_range_map(
        &self,
        api_url: &str,
        key1: &str,
        key2: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        transaction::get_transaction_by_key_range_map(api_url, key1, key2, map).await
    }

    /** APIs provided for the Blocks endpoint **/

    // Get all blocks of a specified type from the given API endpoint.
    pub async fn get_all_blocks<T>(
        &self, 
        api_url: &str
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        blocks::get_all_blocks(api_url).await
    }

    // Get all blocks with additional parameters using a HashMap.
    pub async fn get_all_blocks_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        blocks::get_all_blocks_map(api_url, map).await
    }

    // Get grouped blocks with a specified batch size.
    pub async fn get_blocks_grouped<T>(
        &self, 
        api_url: &str, 
        _batch_size: &i64
    ) -> Result<Vec<Vec<T>>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned + std::default::Default,
    {
        blocks::get_all_blocks_grouped(api_url, _batch_size).await
    }

    // Get grouped blocks with additional parameters using a HashMap.
    pub async fn get_blocks_grouped_map(
        &self,
        api_url: &str, 
        _batch_size: &i64,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<Vec<HashMap<String, Value>>>, anyhow::Error>
    where
    {
        blocks::get_blocks_grouped_map(api_url, _batch_size, map).await
    }

    // Get blocks within a specified range.
    pub async fn get_blocks_by_range<T>(
        &self,
        api_url: &str,
        range_begin: &i64,
        range_end: &i64,
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        blocks::get_blocks_by_range(api_url, range_begin, range_end).await
    }

    // Get blocks within a specified range with additional parameters using a HashMap.
    pub async fn get_blocks_by_range_map(
        &self,
        api_url: &str,
        range_begin: &i64,
        range_end: &i64,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        blocks::get_blocks_by_range_map(api_url, range_begin, range_end, map).await
    }
}