// main.rs

use resilientdb_rust_sdk::ResDB;

mod models;
use models::Transaction;
use models::Block;

use std::collections::HashMap;
use serde_json::json;

#[tokio::main]
async fn test_transaction_api() {
    let res_db = ResDB::new();

    // Create an instance of Transaction
    let my_struct = res_db.create_object::<Transaction>();

    // Call the asynchronous function to get all transactions
    match res_db.get_all_transactions::<Transaction>("https://crow.resilientdb.com/v1/transactions").await {
        Ok(transactions) => {
            if let Some(first_transaction) = transactions.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_transaction);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    // Call the asynchronous function to get a transaction by ID
    match res_db.get_transaction_by_id::<Transaction>(
        "https://crow.resilientdb.com/v1/transactions",
        "3af5ac7a231b6c219bc61867fa25e654d956b61f3990cb5e747d9a1b4baf568e",
    )
    .await
    {
        Ok(transaction) => {
            // Access fields of the transaction (replace with the desired field)
            println!("{:?}", transaction);
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    // Call the asynchronous function to get transactions by key range
    match res_db
        .get_transaction_by_key_range::<Transaction>("https://crow.resilientdb.com/v1/transactions", "10", "60")
        .await
    {
        Ok(transactions) => {
            // Access fields of the transactions (replace with the desired field)
            for transaction in transactions {
                println!("{:?}", transaction);
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }
}

#[tokio::main]
async fn test_transaction_api_map() {
    let data_map = HashMap::new();
    let res_db = ResDB::new();
    
    match res_db.get_all_transactions_map(
        "https://crow.resilientdb.com/v1/transactions",
        data_map,
    )
    .await
    {
        Ok(map) => {
            // Access fields of the transaction (replace with the desired field)
            println!("{:?}", map[10]);
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    let data_map = HashMap::new();
    match res_db.get_transaction_by_id_map(
        "https://crow.resilientdb.com/v1/transactions",
        "3af5ac7a231b6c219bc61867fa25e654d956b61f3990cb5e747d9a1b4baf568e",
        data_map,
    )
    .await
    {
        Ok(map) => {
            // Access fields of the transaction (replace with the desired field)
            println!("{:?}", map);
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    let data_map = HashMap::new();
    match res_db.get_transaction_by_key_range_map(
        "https://crow.resilientdb.com/v1/transactions",
        "3af5ac7a231b6c219bc61867fa25e654d956b61f3990cb5e747d9a1b4baf568e",
        "2e11fb1f6d235a49f4f2a85c6c57f2ddc9a650fb0f0a6e88ea469584294c03cb",
        data_map,
    )
    .await
    {
        Ok(map) => {
            // Access fields of the transaction (replace with the desired field)
            println!("{:?}", map);
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }
}


#[tokio::main]
async fn test_blocks_api() {
    let res_db = ResDB::new();

    // Create an instance of Block
    let my_struct = res_db.create_object::<Block>();

    // Call the asynchronous function to get all transactions
    match res_db.get_all_blocks::<Block>("https://crow.resilientdb.com/v1/blocks").await {
        Ok(blocks) => {
            if let Some(first_block) = blocks.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_block);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    // Call the asynchronous function to get a transaction by ID
    match res_db.get_blocks_grouped::<Block>("https://crow.resilientdb.com/v1/blocks", &5)
    .await
    {
        Ok(blocks) => {
            if let Some(first_block) = blocks.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_block);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }
}

#[tokio::main]
async fn test_blocks_api_map() {
    let res_db = ResDB::new();
    let data_map = HashMap::new();

    // Call the asynchronous function to get blocks
    match res_db
        .get_all_blocks_map("https://crow.resilientdb.com/v1/blocks", data_map)
        .await
    {
        Ok(blocks) => {
            if let Some(first_block) = blocks.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_block);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    let data_map = HashMap::new();
    match res_db
        .get_blocks_by_range_map("https://crow.resilientdb.com/v1/blocks", &1, &10, data_map)
        .await
    {
        Ok(blocks) => {
            if let Some(first_block) = blocks.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_block);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }

    let data_map = HashMap::new();
    match res_db
        .get_blocks_grouped_map("https://crow.resilientdb.com/v1/blocks", &10, data_map)
        .await
    {
        Ok(blocks) => {
            if let Some(first_block) = blocks.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_block);
            } else {
                println!("No transactions available");
            }
        }
        Err(error) => {
            // Handle the error
            eprintln!("Error: {}", error);
        }
    }
}

fn test_crypto() {
    let res_db = ResDB::new();
    let keypair = res_db.generate_keypair(2048);
    println!("{:?}", keypair)
}

#[tokio::main]
async fn test_post() {
    let res_db = ResDB::new();

    let mut data = HashMap::new();
    data.insert(
        "query",
        json!({
            "mutation": {
                "postTransaction": {
                    "data": {
                        "operation": "CREATE",
                        "amount": 69,
                        "signerPublicKey": "8fPAqJvAFAkqGs8GdmDDrkHyR7hHsscVjes39TVVfN54",
                        "signerPrivateKey": "5R4ER6smR6c6fsWt3unPqP6Rhjepbn82Us7hoSj5ZYCc",
                        "recipientPublicKey": "ECJksQuF9UWi3DPCYvQqJPjF6BqSbXrnDiXUjdiVvkyH",
                        "asset": {
                            "data": { "time": 444 }
                        }
                    }
                }
            }
        }),
    );

    let endpoint = "https://cloud.resilientdb.com/graphql";

    match res_db.post_transaction_map(data, endpoint).await {
        Ok(body) => println!("{}", body),
        Err(err) => eprintln!("Error: {}", err),
    }

}

fn main(){
    // test_transaction_api();
    // test_transaction_api_map();
    // test_blocks_api();
    // test_blocks_api_map();
    // test_crypto();
    test_post()
}
