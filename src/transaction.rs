// transaction.rs

#![allow(unused_imports)]
#![allow(dead_code)]
#![allow(unused_variables)]

use std::fs::File;
use std::io::Read;
use serde::Serialize; 
use serde::Serializer;
use serde::Deserialize;
use anyhow::Error;
use serde_json::Value;
use serde_json::json;
use reqwest::{header, StatusCode};
use reqwest::blocking::Client;
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

/// Post transaction to an endpoint using a struct
pub async fn post_transaction<T>(
    _data: T,
    _endpoint: &str,
) -> Result<String, reqwest::Error>
where
    T: Serialize,
{
    let client = reqwest::Client::builder().build()?;
    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert(
        header::CONTENT_TYPE,
        header::HeaderValue::from_static("application/json"),
    );
    let json = json!(_data);
    let response = client
        .post(_endpoint)
        .headers(headers)
        .json(&json)
        .send()
        .await?;

    let body = response.text().await?;
    Ok(body)
}

/// Post transaction to an endpoint using a map
pub async fn post_transaction_map(
    _data: HashMap<&str, Value>,
    _endpoint: &str,
) -> Result<String, reqwest::Error>
{
    let client = reqwest::Client::builder().build()?;
    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert(
        header::CONTENT_TYPE,
        header::HeaderValue::from_static("application/json"),
    );
    let json = json!(_data);
    let request = client.request(reqwest::Method::POST, _endpoint)
        .headers(headers)
        .json(&json);

    let response = request.send().await?;
    let body = response.text().await?;
    Ok(body)
}