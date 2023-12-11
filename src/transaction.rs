// transaction.rs

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

use reqwest::{header, StatusCode, Error as otherError, blocking::Client};
use serde::{Serialize, Serializer, Deserialize}; 
use serde_json::{Map, Value, json};
use std::fs::File;
use std::io::Read;
use anyhow::Error;
use std::collections::HashMap;

/// A testing function that reads JSON data from a file.
/// Note: This function is intended for testing purposes and should not be used in production.
pub async fn get_json_from_file(file_name: &str) -> Result<Vec<Value>, anyhow::Error> {
    let mut file = File::open(file_name)
        .expect("Unable to open the file");
    
    let mut content = String::new();
    file.read_to_string(&mut content).expect("Unable to read the file");
    let json_array: Result<Vec<Value>, _> = serde_json::from_str(&content);
    json_array.map_err(|err| anyhow::Error::from(err))
}

/// Fetch all transactions from the specified API endpoint.
pub async fn get_all_transactions<T>(_api_url: &str) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    let endpoint_url = format!("{}", _api_url);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<T> = response.json().await?;
    Ok(transactions)
}

/// Fetch all transactions with additional parameters using a HashMap.
pub async fn get_all_transactions_map(
    _api_url: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}", _api_url);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(transactions)
}

/// Fetch a specific transaction by ID.
pub async fn get_transaction_by_id<T>(_api_url: &str, _id: &str) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    let endpoint_url = format!("{}/{}", _api_url, _id);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<T> = response.json().await?;
    Ok(transactions)
}

/// Fetch a specific transaction by ID with additional parameters using a HashMap.
pub async fn get_transaction_by_id_map(
    _api_url: &str, 
    _id: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}", _api_url, _id);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(transactions)
}

/// Fetch transactions within a specified key range.
pub async fn get_transaction_by_key_range<T> (
    _api_url: &str,
    _key1: &str,
    _key2: &str,
) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    let endpoint_url = format!("{}/{}/{}", _api_url, _key1, _key2);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<T> = response.json().await?;
    Ok(transactions)
}

/// Fetch transactions within a specified key range with additional parameters using a HashMap.
pub async fn get_transaction_by_key_range_map(
    _api_url: &str, 
    _key1: &str, 
    _key2: &str, 
    _map: HashMap<&str, &str>
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}/{}", _api_url, _key1, _key2);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(transactions)
}

// Post transaction to an endpoint using a struct
// pub async fn post_transaction<T>(
//     _data: T,
//     _endpoint: &str,
// ) -> Result<String, anyhow::Error>
// where
//     T: Serialize,
// {
//     let client = reqwest::Client::builder().build()?;

//     let mut headers = reqwest::header::HeaderMap::new();
//     headers.insert("Content-Type", "application/json".parse()?);

//     let request = client
//         .post(_endpoint)
//         .headers(headers)
//         .json(&_data);

//     let response = request.send().await?;
//     let body = response.text().await?;

//     Ok(body)
// }

// Post transaction to an endpoint using a map
// pub async fn post_transaction_map(
//     _data: HashMap<&str, Value>,
//     _endpoint: &str,
// ) -> Result<String, anyhow::Error>
// {
//     let json: serde_json::Value = serde_json::Value::Object(
//         _data.into_iter()
//             .map(|(k, v)| (k.to_string(), v))
//             .collect::<Map<String, Value>>(),
//     );

//     let json = serde_json::to_string_pretty(&json)?;

//     println!("{:?}", json);

//     let client = reqwest::Client::builder()
//         .build()?;

//     let mut headers = reqwest::header::HeaderMap::new();
//     headers.insert("Content-Type", "application/json".parse()?);

//     let request = client.request(reqwest::Method::POST, _endpoint)
//         .headers(headers)
//         .json(&json);

//     let response = request.send().await?;
//     let body = response.text().await?;
    
//     Ok(body)
// }

pub async fn post_transaction_string(
    _data: &str,
    _endpoint: &str,
) -> Result<String, anyhow::Error>
where
{
    let json: serde_json::Value = match serde_json::from_str(_data) {
        Ok(value) => value,
        Err(err) => return Err(Error::from(err)),
    };

    println!("{}", json);

    let client = reqwest::Client::builder()
        .build()?;

    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert("Content-Type", "application/json".parse()?);


    let request = client.request(reqwest::Method::POST, _endpoint)
        .headers(headers)
        .json(&json);

    let response = request.send().await?;
    let body = response.text().await?;

    Ok(body)
}