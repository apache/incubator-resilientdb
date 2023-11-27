// blocks.rs

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

use std::fs::File;
use std::io::Read;
use serde::Deserialize;
use anyhow::Error;
use serde_json::Value;
use reqwest::blocking::Client;
use reqwest::StatusCode;
use std::collections::HashMap;

pub async fn get_json_from_file(file_name: &str) -> Result<Vec<Value>, anyhow::Error> {
    let mut file = File::open(file_name)
        .expect("Unable to open the file");
    
    // Read the file content into a string
    let mut content = String::new();
    file.read_to_string(&mut content).expect("Unable to read the file");

    // Parse the JSON content
    let json_array: Result<Vec<Value>, _> = serde_json::from_str(&content);
    
    // Return the result
    json_array.map_err(|err| anyhow::Error::from(err))
}


pub async fn get_all_blocks<T>(_api_url: &str) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(&*_api_url).await?;
    let transactions: Vec<T> = response.json().await?;
    Ok(transactions)
}

pub async fn get_all_blocks_map(
    _api_url: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(_api_url).await?;
    let blocks: Vec<HashMap<String, Value>> = response.json().await?;

    // let json_array = get_json_from_file("/mnt/c/Users/dsang/OneDrive/Desktop/resdb_rust_sdk/json_data/transactions:id.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    // let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(blocks)
}

pub async fn get_all_blocks_grouped<T>(_api_url: &str, _batch_size: &i64) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned + std::default::Default,
{
    // append _id to _url
    let endpoint_url = format!("{}/{}", _api_url, _batch_size);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<T> = response.json().await?;
    Ok(blocks)
}

pub async fn get_blocks_grouped_map(
    _api_url: &str, 
    _batch_size : &i64,
    _map: HashMap<&str, &str>
) -> Result<Vec<Vec<HashMap<String, Value>>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}", _api_url, _batch_size);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<Vec<HashMap<String, Value>>> = response.json().await?;

    // let json_array = get_json_from_file("/mnt/c/Users/dsang/OneDrive/Desktop/resdb_rust_sdk/json_data/transactions:id.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    // let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(blocks)
}

pub async fn get_blocks_by_range<T>(
    _api_url: &str,
    _range_begin: &i64,
    _range_end: &i64,
) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // append _key1 & _key2 to _url
    let endpoint_url = format!("{}/{}/{}", _api_url, _range_begin, _range_end);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<T> = response.json().await?;

    Ok(blocks)
}

pub async fn get_blocks_by_range_map(
    _api_url: &str, 
    _range_begin: &i64, 
    _range_end: &i64, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}/{}", _api_url, _range_begin, _range_end);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<HashMap<String, Value>> = response.json().await?;

    // let json_array = get_json_from_file("/mnt/c/Users/dsang/OneDrive/Desktop/resdb_rust_sdk/json_data/transactions:id.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    // let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(blocks)
}



