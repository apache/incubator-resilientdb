// resdb.rs
use crate::transaction::Transaction;
// use crate::blocks::Blocks;


pub struct ResDB {
    pub database_url: String,
}

impl ResDB {

    //constructor for the ResDB object
    pub fn new(database_url: &str) -> Self {
        // Implementation for creating a new ResDB instance
        ResDB {
            database_url: database_url.to_string(),
        }
    }

    // all functions relating to the transactions endpoints
    pub fn create_transaction(&self) -> Transaction {
        Transaction::new()
    }

    pub async fn get_all_transactions(&self) -> Result<Vec<Transaction>, anyhow::Error> {
        Transaction::get_all_transactions(&self.database_url).await
    }

    pub async fn get_transaction_by_id(&self, id: &str) {
        let transactions = Transaction::get_transaction_by_id(&self.database_url);
    }


    // pub fn get_transaction_by_key_range(&self, key1: &str, key2: &str) {
    //     let transaction = Transaction::new();  // Create a new instance of Transaction
    //     transaction.get_transaction_by_key_range(&self.database_url, key1, key2);
    // }

    // pub fn commit_transaction(&self) {
    //     let transaction = Transaction::new();  // Create a new instance of Transaction
    //     transaction.commit_transaction(&self.database_url);
    // }

    // functions relating to the blocks endpoints: TO-DO!

    // // retrive all the blocks within the chain
    // pub fn get_all_blocks(&self) {
    //     blocks.commit_transaction(&self.database_url);
    // }

    // // retrive blocks in batch sizes, order remains the same
    // pub fn get_blocks_by_batch(&self, size: &[int32]) {
    //     blocks.commit_transaction(&self.database_url);
    // }

    // // retrive blocks within the chain within a range. Ex. 1 -> 10
    // pub fn get_blocks_by_range(&self, int32: size) {
    //     blocks.commit_transaction(&self.database_url);
    // }
}


