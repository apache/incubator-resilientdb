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
use reqwest::blocking::Client;
use reqwest::StatusCode;
use serde::Deserialize;
use serde_json::Value;
use std::collections::HashMap;
use std::fs::File;
use std::io::Read;

/// A testing function that reads JSON data from a file.
/// Note: This function is intended for testing purposes and should not be used in production.
pub async fn get_json_from_file(file_name: &str) -> Result<Vec<Value>, anyhow::Error> {
    let mut file = File::open(file_name).expect("Unable to open the file");

    // Read the file content into a string
    let mut content = String::new();
    file.read_to_string(&mut content)
        .expect("Unable to read the file");

    // Parse the JSON content
    let json_array: Result<Vec<Value>, _> = serde_json::from_str(&content);

    // Return the result
    json_array.map_err(|err| anyhow::Error::from(err))
}

/// Fetch all blocks from the specified API endpoint.
pub async fn get_all_blocks<T>(_api_url: &str) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(&*_api_url).await?;
    let blocks: Vec<T> = response.json().await?;
    Ok(blocks)
}

/// Fetch all blocks with additional parameters using a HashMap.
pub async fn get_all_blocks_map(
    _api_url: &str,
    _map: HashMap<&str, &str>,
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(_api_url).await?;
    let blocks: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(blocks)
}

/// Fetch grouped blocks with a specified batch size.
pub async fn get_all_blocks_grouped<T>(
    _api_url: &str,
    _batch_size: &i64,
) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned + std::default::Default,
{
    // Append _batch_size to _api_url
    let endpoint_url = format!("{}/{}", _api_url, _batch_size);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<T> = response.json().await?;
    Ok(blocks)
}

/// Fetch grouped blocks with additional parameters using a HashMap.
pub async fn get_blocks_grouped_map(
    _api_url: &str,
    _batch_size: &i64,
    _map: HashMap<&str, &str>,
) -> Result<Vec<Vec<HashMap<String, Value>>>, anyhow::Error>
where
{
    // Append _batch_size to _api_url
    let endpoint_url = format!("{}/{}", _api_url, _batch_size);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<Vec<HashMap<String, Value>>> = response.json().await?;
    Ok(blocks)
}

/// Fetch blocks within a specified range.
pub async fn get_blocks_by_range<T>(
    _api_url: &str,
    _range_begin: &i64,
    _range_end: &i64,
) -> Result<Vec<T>, anyhow::Error>
where
    T: serde::de::DeserializeOwned,
{
    // Append _range_begin and _range_end to _api_url
    let endpoint_url = format!("{}/{}/{}", _api_url, _range_begin, _range_end);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<T> = response.json().await?;
    Ok(blocks)
}

/// Fetch blocks within a specified range with additional parameters using a HashMap.
pub async fn get_blocks_by_range_map(
    _api_url: &str,
    _range_begin: &i64,
    _range_end: &i64,
    _map: HashMap<&str, &str>,
) -> Result<Vec<HashMap<String, Value>>, anyhow::Error>
where
{
    // Append _range_begin and _range_end to _api_url
    let endpoint_url = format!("{}/{}/{}", _api_url, _range_begin, _range_end);

    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(endpoint_url).await?;
    let blocks: Vec<HashMap<String, Value>> = response.json().await?;
    Ok(blocks)
}
