// This is the main entry point for the library.

use reqwest::Error;
use serde::Deserialize;

#[derive(Debug, Deserialize)]
struct ApiResponse {
    inputs: Vec<Input>,
    outputs: Vec<Output>,
    operation: String,
    metadata: Option<serde_json::Value>,
    asset: Asset,
    version: String,
    id: String,
}

#[derive(Debug, Deserialize)]
struct Input {
    owners_before: Vec<String>,
    fulfills: Option<serde_json::Value>,
    fulfillment: String,
}

#[derive(Debug, Deserialize)]
struct Output {
    public_keys: Vec<String>,
    condition: Condition,
    amount: String,
}

#[derive(Debug, Deserialize)]
struct Condition {
    details: ConditionDetails,
    uri: String,
}

#[derive(Debug, Deserialize)]
struct ConditionDetails {
    #[serde(rename = "type")]
    condition_type: String,
    public_key: String,
}

#[derive(Debug, Deserialize)]
struct Asset {
    data: serde_json::Value,
}

pub async fn get_data_from_api(api_url: &str) -> Result<ApiResponse, Error> {
    // Make an asynchronous GET request to the specified API endpoint
    let response = reqwest::get(api_url).await?;

    // Check if the response has a successful status code
    response.error_for_status()?;

    // If successful, deserialize the JSON response
    let response_text = response.text().await?;
    let response: ApiResponse = serde_json::from_str(&response_text)?;

    Ok(response)
}
