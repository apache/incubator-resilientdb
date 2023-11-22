// main.rs

use resdb_rust_sdk::ResDB;
mod models;
use models::Transaction;
use models::Block;

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
        "8fPAqJvAFAkqGs8GdmDDrkHyR7hHsscVjes39TVVfN54",
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
    match res_db.get_all_blocks_grouped::<Block>("https://crow.resilientdb.com/v1/blocks", &5)
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

    // Call the asynchronous function to get transactions by key range
    match res_db
        .get_blocks_by_range::<Block>("https://crow.resilientdb.com/v1/blocks", &1, &10)
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

fn main(){
    // test_transaction_api();
    test_blocks_api()
}