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

use anyhow::Error;
use reqwest::{blocking::Client, header, Error as otherError, StatusCode};
use serde::{Deserialize, Serialize, Serializer};
use serde_json::{json, Map, Value};
use std::collections::HashMap;
use std::fs::File;
use std::io::Read;

/// A testing function that reads JSON data from a file.
/// Note: This function is intended for testing purposes and should not be used in production.
pub async fn get_json_from_file(file_name: &str) -> Result<Vec<Value>, anyhow::Error> {
    let mut file = File::open(file_name).expect("Unable to open the file");

    let mut content = String::new();
    file.read_to_string(&mut content)
        .expect("Unable to read the file");
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
    _map: HashMap<&str, &str>,
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
    _map: HashMap<&str, &str>,
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}", _api_url, _id);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(transactions)
}

/// Fetch transactions within a specified key range.
pub async fn get_transaction_by_key_range<T>(
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
    _map: HashMap<&str, &str>,
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    let endpoint_url = format!("{}/{}/{}", _api_url, _key1, _key2);
    let response = reqwest::get(endpoint_url).await?;
    let transactions: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(transactions)
}

/// Post transaction given a JSON string.
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

    let client = reqwest::Client::builder().build()?;

    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert("Content-Type", "application/json".parse()?);

    let request = client
        .request(reqwest::Method::POST, _endpoint)
        .headers(headers)
        .json(&json);

    let response = request.send().await?;
    let body = response.text().await?;

    Ok(body)
}
