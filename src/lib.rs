// This is the main entry point for the library.

// use reqwest::Error;
use serde::Deserialize;
use anyhow::Error;

#[derive(Debug, Deserialize)]
pub struct ApiResponse {
    inputs: Vec<Input>,
    outputs: Vec<Output>,
    operation: String,
    metadata: Option<serde_json::Value>,
    asset: Asset,
    version: String,
    id: String,
}

#[derive(Debug, Deserialize)]
pub struct Input {
    owners_before: Vec<String>,
    fulfills: Option<serde_json::Value>,
    fulfillment: String,
}

#[derive(Debug, Deserialize)]
pub struct Output {
    public_keys: Vec<String>,
    condition: Condition,
    amount: String,
}

#[derive(Debug, Deserialize)]
pub struct Condition {
    details: ConditionDetails,
    uri: String,
}

#[derive(Debug, Deserialize)]
pub struct ConditionDetails {
    #[serde(rename = "type")]
    condition_type: String,
    public_key: String,
}

#[derive(Debug, Deserialize)]
pub struct Asset {
    data: serde_json::Value,
}

pub async fn get_data_from_api(api_url: &str) -> Result<ApiResponse, anyhow::Error> {
    // Make an asynchronous GET request to the specified API endpoint
    let response_text = reqwest::get(api_url)
        .await?
        .text()
        .await?;

    println!("{}", response_text);
    // If successful, deserialize the JSON response
    let response: Result<ApiResponse, serde_json::Error> = serde_json::from_str(&response_text);

    // Handle the deserialization result
    match response {
        Ok(parsed_response) => Ok(parsed_response),
        Err(err) => Err(anyhow::Error::from(err)),
    }
}
