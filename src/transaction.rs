// transaction.rs

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


pub async fn get_all_transactions<T>(_api_url: &str) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // Make an asynchronous GET request to the specified API endpoint
    // let response = reqwest::get(&*_api_url).await?;
    // let transactions: Vec<T> = response.json().await?;

    let json_array = get_json_from_file("/Users/dhruv/Desktop/Repositories/resdb/resdb_rust_sdk/json_data/transactions.json").await?;
    let mut transactions = Vec::new();

    for json_obj in json_array{
        let transaction: T = serde_json::from_value(json_obj)?;
        transactions.push(transaction);
    }

    Ok(transactions)
}

pub async fn get_all_transactions_map(
    _api_url: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    // Make an asynchronous GET request to the specified API endpoint
    // let response = reqwest::get(&*_api_url).await?;
    // let transactions: Vec<T> = response.json().await?;

    let json_array = get_json_from_file("/Users/dhruv/Desktop/Repositories/resdb/resdb_rust_sdk/json_data/transactions.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(transactions)
}

pub async fn get_transaction_by_id<T>(_api_url: &str, _id: &str) -> Result<T, anyhow::Error>
where
    T: serde::de::DeserializeOwned + std::default::Default,
{
    // append _id to _url
    // let endpoint_url = format!("{}/{}", _api_url, _id);

    // Make an asynchronous GET request to the specified API endpoint
    // let response = reqwest::get(endpoint_url).await?;
    // let transactions: Vec<T> = response.json().await?;

    let json_array = get_json_from_file("/Users/dhruv/Desktop/Repositories/resdb/resdb_rust_sdk/json_data/transactions:id.json").await?;

    // Check if there's a transaction with the specified ID
    if let Some(json_obj) = json_array.into_iter().next() {
        let transaction: T = serde_json::from_value(json_obj)?;
        Ok(transaction)
    } else {
        // If no transaction is found, return the default value
        Ok(T::default())
    }
}

pub async fn get_transaction_by_id_map(
    _api_url: &str, 
    _id: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    // Make an asynchronous GET request to the specified API endpoint
    // let response = reqwest::get(&*_api_url).await?;
    // let transactions: Vec<T> = response.json().await?;

    let json_array = get_json_from_file("/Users/dhruv/Desktop/Repositories/resdb/resdb_rust_sdk/json_data/transactions:id.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(transactions)
}

pub async fn get_transaction_by_key_range<T> (
    _api_url: &str,
    _key1: &str,
    _key2: &str,
) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // append _key1 & _key2 to _url
    let endpoint_url = format!("{}/{}/{}", _api_url, _key1, _key2);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<T> = response.json().await?;

    // let json_array = get_json_from_file("/mnt/c/Users/dsang/OneDrive/Desktop/resdb_rust_sdk/json_data/transactions:id.json").await?;

    // // Check if there's a transaction with the specified ID
    // if let Some(json_obj) = json_array.into_iter().next() {
    //     let transaction: T = serde_json::from_value(json_obj)?;
    //     Ok(transaction)
    // } else {
    //     // If no transaction is found, return the default value
    //     Ok(T::default())
    // }

    Ok(transactions)
}

pub async fn get_transaction_by_key_range_map(
    _api_url: &str, 
    _key1: &str, 
    _key2: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}/{}", _api_url, _key1, _key2);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;

    // let json_array = get_json_from_file("/mnt/c/Users/dsang/OneDrive/Desktop/resdb_rust_sdk/json_data/transactions:id.json").await?;
    // let mut transactions = Vec::new();

    // Deserialize JSON into a vector of HashMaps
    // let transactions: Vec<HashMap<String, Value>> = serde_json::from_value(serde_json::Value::Array(json_array))?;
    Ok(transactions)
}



