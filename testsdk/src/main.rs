// This is the main file for a project that uses the library.
extern crate resdb_rust_sdk;
use tokio::main;

#[tokio::main]
async fn main() {
    // Specify the URL of the JSON API endpoint
    let api_url = "https://crow.resilientdb.com/v1/transactions";

    match resdb_rust_sdk::get_data_from_api(api_url).await {
        Ok(response) => {
            println!("Response: {:?}", response);
            // Handle the response data as needed
        }
        Err(err) => {
            eprintln!("Error: {:?}", err);
            // Handle the error
        }
    }
}
