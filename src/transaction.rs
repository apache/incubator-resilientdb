// transaction.rs
// use reqwest::Error;
use std::fs::File;
use std::io::Read;
use serde::Deserialize;
use anyhow::Error;

use super::ResDB;

#[derive(Debug, Deserialize)]
pub struct Transaction {
    pub inputs: Vec<Input>,
    pub outputs: Vec<Output>,
    pub operation: String,
    pub metadata: Option<serde_json::Value>,
    pub asset: Asset,
    pub version: String,
    pub id: String,
}

#[derive(Debug, Deserialize)]
pub struct Input {
    pub owners_before: Vec<String>,
    pub fulfills: Option<serde_json::Value>,
    pub fulfillment: String,
}

#[derive(Debug, Deserialize)]
pub struct Output {
    pub public_keys: Vec<String>,
    pub condition: Condition,
    pub amount: String,
}

#[derive(Debug, Deserialize)]
pub struct Condition {
    pub details: ConditionDetails,
    pub uri: String,
}

#[derive(Debug, Deserialize)]
pub struct ConditionDetails {
    #[serde(rename = "type")]
    pub condition_type: String,
    pub public_key: String,
}

#[derive(Debug, Deserialize)]
pub struct Asset {
    pub data: serde_json::Value,
}

impl Transaction {
    
    pub fn new() -> Self {
        // Implementation for creating a new Transaction instance
        Transaction {
            inputs: Vec::new(),
            outputs: Vec::new(),
            operation: String::new(),
            metadata: None,
            asset: Asset { data: serde_json::Value::Null },
            version: String::new(),
            id: String::new(),
        }
    }

    // implementation of GET v1/transactions
    pub async fn get_all_transactions(api_url: &str) -> Result<Vec<Transaction>, anyhow::Error> {
        // Make an asynchronous GET request to the specified API endpoint
        let response_text = reqwest::get(api_url).await?;
        
        // Open the file -> REPLACE THIS CODE
        let mut file = File::open("/Users/dhruv/Desktop/Repositories/resdb_rust_sdk/json_data/transactions.json").expect("Unable to open the file");
        // Read the file content into a string
        let mut content = String::new();
        file.read_to_string(&mut content).expect("Unable to read the file");
    
    
        let json_array: Vec<serde_json::Value> = serde_json::from_str(&content)?;
        let mut transactions = Vec::new();
    
        for json_obj in json_array{
            let transaction: Transaction = serde_json::from_value(json_obj)?;
            transactions.push(transaction);
        }
        
        // let response: Result<Transaction, serde_json::Error> = serde_json::from_str(&content);
        let response: Result<Vec<Transaction>, serde_json::Error> = Ok(transactions);
    
        // Handle the deserialization result
        match response {
            Ok(parsed_response) => Ok(parsed_response),
            Err(err) => Err(anyhow::Error::from(err)),
        }
    }

    // implementation of GET v1/transactions/<string>    
    pub async fn get_transaction_by_id(api_url: &str) -> Result<Vec<Transaction>, anyhow::Error> {
        // Make an asynchronous GET request to the specified API endpoint
        let response_text = reqwest::get(api_url).await?;
        
        // Open the file -> REPLACE THIS CODE
        let mut file = File::open("/Users/dhruv/Desktop/Repositories/resdb_rust_sdk/json_data/transactions.json").expect("Unable to open the file");
        // Read the file content into a string
        let mut content = String::new();
        file.read_to_string(&mut content).expect("Unable to read the file");
    
    
        let json_array: Vec<serde_json::Value> = serde_json::from_str(&content)?;
        let mut transactions = Vec::new();
    
        for json_obj in json_array{
            let transaction: Transaction = serde_json::from_value(json_obj)?;
            transactions.push(transaction);
        }
        
        // let response: Result<Transaction, serde_json::Error> = serde_json::from_str(&content);
        let response: Result<Vec<Transaction>, serde_json::Error> = Ok(transactions);
    
        // Handle the deserialization result
        match response {
            Ok(parsed_response) => Ok(parsed_response),
            Err(err) => Err(anyhow::Error::from(err)),
        }
    }

    // implementation of GET v1/transactions/<string>/<string>

    // pub async fn get_transaction_by_key_range(api_url: &str) -> Result<Vec<Transaction>, anyhow::Error> {
    // }

    // implementation of POST v1/transactions/commit

    // pub async fn commit_transaction(api_url: &str) -> Result<Vec<Transaction>, anyhow::Error> {

    // }
}



