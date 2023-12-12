
use serde::Deserialize;
/** User Defined Struct for transaction endpoints **/ 

#[derive(Debug, serde::Deserialize)]
pub struct Transaction {
    pub inputs: Vec<Input>,
    pub outputs: Vec<Output>,
    pub operation: String,
    pub metadata: Option<serde_json::Value>,
    pub asset: Asset,
    pub version: String,
    pub id: String,
}

impl Default for Transaction {
    fn default() -> Self {
        Transaction {
            inputs: Vec::new(),
            outputs: Vec::new(),
            operation: String::new(),
            metadata: None,
            asset: Asset::default(),
            version: String::new(),
            id: String::new(),
        }
    }
}

#[derive(Debug, serde::Deserialize)]
pub struct Input {
    pub owners_before: Vec<String>,
    pub fulfills: Option<serde_json::Value>,
    pub fulfillment: String,
}

impl Default for Input {
    fn default() -> Self {
        Input {
            owners_before: Vec::new(),
            fulfills: None,
            fulfillment: String::new(),
        }
    }
}

#[derive(Debug, serde::Deserialize)]
pub struct Output {
    pub public_keys: Vec<String>,
    pub condition: Condition,
    pub amount: String,
}

impl Default for Output {
    fn default() -> Self {
        Output {
            public_keys: Vec::new(),
            condition: Condition::default(),
            amount: String::new(),
        }
    }
}

#[derive(Debug, serde::Deserialize)]
pub struct Condition {
    pub details: ConditionDetails,
    pub uri: String,
}

impl Default for Condition {
    fn default() -> Self {
        Condition {
            details: ConditionDetails::default(),
            uri: String::new(),
        }
    }
}

#[derive(Debug, serde::Deserialize)]
pub struct ConditionDetails {
    #[serde(rename = "type")]
    pub condition_type: String,
    pub public_key: String,
}

impl Default for ConditionDetails {
    fn default() -> Self {
        ConditionDetails {
            condition_type: String::new(),
            public_key: String::new(),
        }
    }
}

#[derive(Debug, serde::Deserialize)]
pub struct Asset {
    pub data: serde_json::Value,
}

impl Default for Asset {
    fn default() -> Self {
        Asset {
            data: serde_json::Value::Null,
        }
    }
}

/** User defined struct for blocks endpoints **/

#[derive(Debug, Deserialize)]
pub struct BlockTransaction {
    pub cmd: String,
    pub key: Option<String>,
    pub value: Option<String>, // Use Option<String> to handle cases where "value" is missing
}

impl Default for BlockTransaction {
    fn default() -> Self {
        BlockTransaction {
            cmd: String::new(),
            key: None,
            value: None,
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct Block {
    pub id: i64,
    pub number: String,
    pub transactions: Vec<BlockTransaction>,
    pub size: i64,
    pub created_at: String,
}

impl Default for Block {
    fn default() -> Self {
        Block {
            id: 0,
            number: String::new(),
            transactions: Vec::new(),
            size: 0,
            created_at: String::new(),
        }
    }
}