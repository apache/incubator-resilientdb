// This is the main file for a project that uses the library.

extern crate resdb_rust_sdk;

fn main() {
    let result = resdb_rust_sdk::add_numbers(5, 7);
    println!("Result: {}", result);
}