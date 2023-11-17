// This is the main file for a project that uses the library.
use resdb_rust_sdk::ResDB;

#[tokio::main]
async fn main() {
    // Specify the URL of the JSON API endpoint
    let database_url = "https://crow.resilientdb.com/v1/transactions";
    let res_db = ResDB::new(database_url);
    let transactions = res_db.get_all_transactions();
    
    // Call the asynchronous function and await the result
    match res_db.get_all_transactions().await {
        Ok(transactions) => {
            if let Some(first_transaction) = transactions.first() {
                // Access fields of the first transaction (replace with the desired field)
                println!("{:?}", first_transaction.inputs);
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
