// resdb.rs

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

use crate::transaction;
use crate::blocks;
use std::collections::HashMap;
use serde_json::Value;

pub struct ResDB;

impl ResDB {
    pub fn new() -> Self {
        ResDB
    }

    /** APIs provided for the transaction endpoint **/
    /** Functions that accept structs **/

    pub fn create_object<T>(&self) -> T
    where
        T: Default,
    {
        T::default()
    }

    pub async fn get_all_transactions<T>(&self, 
        api_url: &str
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        transaction::get_all_transactions(api_url).await
    }

    pub async fn get_all_transactions_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        transaction::get_all_transactions_map(api_url, map).await
    }

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

    pub async fn get_all_blocks<T>(
        &self, 
        api_url: &str
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned,
    {
        blocks::get_all_blocks(api_url).await
    }

    pub async fn get_all_blocks_map(
        &self,
        api_url: &str,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        blocks::get_all_blocks_map(api_url, map).await
    }

    pub async fn get_blocks_grouped<T>(
        &self, 
        api_url: &str, 
        _batch_size: &i64
    ) -> Result<Vec<T>, anyhow::Error>
    where
        T: serde::de::DeserializeOwned + std::default::Default,
    {
        blocks::get_all_blocks_grouped(api_url, _batch_size).await
    }

    pub async fn get_blocks_grouped_map(
        &self,
        api_url: &str, 
        _batch_size: &i64,
        map: HashMap<&str, &str>,
    ) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
    where
    {
        blocks::get_blocks_grouped_map(api_url, _batch_size, map).await
    }

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