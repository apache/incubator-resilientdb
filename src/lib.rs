// lib.rs

/// This crate provides modules for interacting with a resource database, transactions, and blocks.
pub mod resdb;
pub mod transaction;
pub mod blocks;
pub mod crypto;

/// Re-export ResDB from the resdb module for convenient use.
pub use resdb::ResDB;
