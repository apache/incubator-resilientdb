// lib.rs

/// This modeule provides acces to other methods in the rest of the modules.
pub mod resdb;

/// This module provides access to the functions interfacing with the transaction endpoint.
pub mod transaction;

/// This module provides access to the functions interfacing with the block endpoint.
pub mod blocks;

/// This module provides access to functions that allow user to generate public-private keys and hash data.
pub mod crypto;

/// Re-export ResDB from the resdb module for convenient use.
pub use resdb::ResDB;
