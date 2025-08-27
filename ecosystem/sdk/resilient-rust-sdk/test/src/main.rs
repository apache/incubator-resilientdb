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

#![allow(dead_code)]
#![allow(unused_variables)]

use resilientdb_rust_sdk::ResDB;

mod models;
use models::{Transaction, Block};

use std::collections::HashMap;


#[tokio::main]
async fn test_transaction_api() {
    let res_db = ResDB::new();

    // Create an instance of Transaction
    let my_struct = res_db.create_object::<Transaction>();

    // Call the asynchronous function to get all transactions
    match res_db.get_all_transactions::<Transaction>("https://crow.resilientdb.com").await {
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
        "https://crow.resilientdb.com",
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
        .get_transaction_by_key_range::<Transaction>("https://crow.resilientdb.com", "10", "60")
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
        "https://crow.resilientdb.com",
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
        "https://crow.resilientdb.com",
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
        "https://crow.resilientdb.com",
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
    match res_db.get_all_blocks::<Block>("https://crow.resilientdb.com").await {
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
    match res_db.get_blocks_grouped::<Block>("https://crow.resilientdb.com", &5)
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
        .get_all_blocks_map("https://crow.resilientdb.com", data_map)
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
        .get_blocks_by_range_map("https://crow.resilientdb.com", &1, &10, data_map)
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
        .get_blocks_grouped_map("https://crow.resilientdb.com", &10, data_map)
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

fn test_crypto() -> (String, String){
    let res_db = ResDB::new(); 

    res_db.generate_keypair()
}

#[tokio::main]
async fn test_post( keypair : (String, String) ) {
    let res_db = ResDB::new();

    let data = format!(r#"
    {{
        "query": "mutation {{ postTransaction(data: {{\noperation: \"CREATE\"\namount: 173812\nsignerPublicKey: \"{}\"\nsignerPrivateKey: \"{}\"\nrecipientPublicKey: \"ECJksQuF9UWi3DPCYvQqJPjF6BqSbXrnDiXUjdiVvkyH\"\nasset: \"\"\"{{\n            \"data\": {{ \"time\": 173812\n            }},\n          }}\"\"\"\n      }}) {{\n  id\n  }}\n}}\n"
    }}
    "#, keypair.0, keypair.1);

    let endpoint = "https://cloud.resilientdb.com/graphql";
    match res_db.post_transaction_string(&data, endpoint).await {
        Ok(body) => println!("{}", body),
        Err(err) => eprintln!("Error: {}", err),
    }
}

fn main(){
    test_transaction_api();
    test_transaction_api_map();
    test_blocks_api();
    test_blocks_api_map();
    let keypair = test_crypto();
    println!("Public Key: {:?}", keypair.0);
    println!("Private Key: {:?}", keypair.1);
    test_post(keypair)
}
